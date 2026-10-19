"""Collect license texts/attributions from the exact Qt source tags used in CI.

The aqt Windows binary kit does not include a top-level license directory.
Keep the source URLs alongside the extracted license and attribution files.
"""
import json
from pathlib import Path, PurePosixPath
import shutil
import sys
import tarfile
import tempfile
import urllib.request
from concurrent.futures import ThreadPoolExecutor

VERSION = "6.8.3"
MODULES = ("qtbase", "qtdeclarative", "qtsvg", "qtimageformats", "qtshadertools", "qttranslations")
DESTINATION = Path(sys.argv[1])


def collect(module):
    url = f"https://codeload.github.com/qt/{module}/tar.gz/refs/tags/v{VERSION}"
    destination = DESTINATION / module
    destination.mkdir(parents=True, exist_ok=True)
    (destination / "SOURCE.txt").write_text(url + "\n", encoding="utf-8")
    with tempfile.TemporaryFile() as archive:
        request = urllib.request.Request(url, headers={"User-Agent": "Lantern-Windows-build"})
        with urllib.request.urlopen(request, timeout=120) as response:
            shutil.copyfileobj(response, archive)
        archive.seek(0)
        with tarfile.open(fileobj=archive, mode="r:gz") as source:
            members = {item.name: item for item in source.getmembers() if item.isfile()}
            selected = set()
            for name, member in members.items():
                relative = PurePosixPath(name)
                basename = relative.name.lower()
                if ("LICENSES" in relative.parts or basename.startswith(("license", "licence", "copying", "copyright", "notice"))
                        or basename == "qt_attribution.json"):
                    selected.add(name)
                if basename == "qt_attribution.json" and "tests" not in relative.parts:
                    # Qt metadata can contain literal newlines inside license strings.
                    metadata = json.load(source.extractfile(member), strict=False)
                    for item in metadata if isinstance(metadata, list) else [metadata]:
                        # Some Qt attributions refer to license-bearing source files.
                        for license_file in item.get("LicenseFile", "").split():
                            target = str(relative.parent / license_file)
                            if target in members:
                                selected.add(target)
            for name in sorted(selected):
                # Never extract archive paths directly or follow links.
                relative = PurePosixPath(name).parts[1:]
                if not relative or ".." in relative:
                    raise ValueError(f"Unsafe archive member: {name}")
                output = destination.joinpath(*relative)
                output.parent.mkdir(parents=True, exist_ok=True)
                output.write_bytes(source.extractfile(members[name]).read())
            if not (destination / "LICENSES" / "LGPL-3.0-only.txt").exists() and module in ("qtbase", "qtdeclarative"):
                raise RuntimeError(f"LGPL license text missing for {module}")
    print(f"Collected {len(selected)} license/attribution files for {module}", flush=True)


with ThreadPoolExecutor(max_workers=3) as workers:
    list(workers.map(collect, MODULES))
