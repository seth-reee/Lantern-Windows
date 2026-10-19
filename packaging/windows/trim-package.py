"""Remove optional styles and tooling from a Basic-style-only Qt deployment."""
from pathlib import Path
import shutil
import sys

root = Path(sys.argv[1])
if not (root / 'lantern.exe').is_file():
    raise SystemExit('Expected a deployed Lantern directory')
# Qt's dialog import graph can include all style variants even though Lantern
# explicitly imports Basic. Their plugins and matching DLLs are unused at runtime.
styles = ('Fusion', 'Imagine', 'Material', 'Universal', 'FluentWinUI3', 'Windows', 'macOS', 'iOS')
for style in styles:
    directory = root / 'qml/QtQuick/Controls' / style
    if directory.exists():
        shutil.rmtree(directory)
    for library in root.glob(f'Qt6QuickControls2{style}*.dll'):
        library.unlink()
native_style = root / 'qml/QtQuick/NativeStyle'
if native_style.exists():
    shutil.rmtree(native_style)
# Type descriptions are used by IDEs; the QML runtime uses qmldir and plugins.
for description in root.rglob('*.qmltypes'):
    description.unlink()
for excluded in ('translations', 'qmltooling', 'qt-licenses'):
    if (root / excluded).exists():
        shutil.rmtree(root / excluded)
(root / 'opengl32sw.dll').unlink(missing_ok=True)
