# <img src="Lantern.png" width="48" height="48" alt=""> Lantern for Windows

A lightweight IPv4 network scanner built with C++20 and Qt Quick, ported from
[Lantern for Linux](https://github.com/seth-reee/Lantern).

## Windows preview

Target: Windows 11 x64. The Windows workflow builds with MSVC 2022 and Qt 6.8.3,
runs tests including native ICMP loopback, and packages a portable ZIP with Qt
libraries, plugins, and QML dependencies.

Published versions are available on [Releases](https://github.com/seth-reee/Lantern-Windows/releases)
once a version tag is published. For development test builds, open [Actions](https://github.com/seth-reee/Lantern-Windows/actions/workflows/windows.yml),
select a successful Windows build, and download the **Lantern-Windows-x64** artifact
(GitHub sign-in required). Extract the artifact, then extract its
`Lantern-Windows-x64.zip` and run `Lantern-Windows/lantern.exe`.
Keep the executable with the accompanying folders and DLLs. No Qt installation
or application installer is needed. The ZIP includes the Microsoft C++ runtime
DLLs beside the executable; Windows supplies standard OS DLLs. Preview builds are unsigned.

Launch and scanning were confirmed by the user in a Windows test VM on
2026-10-19. Theme changes, high DPI, hardware rendering, and detailed network
results still need manual checking.

## Features

- Scan an IPv4 address, inclusive range, or CIDR (up to 65,536 targets).
- Native Windows ICMP discovery without parsing localized ping output.
- Optional checks of 23 common TCP ports and reverse DNS lookup.
- Available local-network MAC addresses through the Windows neighbor table.
- Search, status filters, sorting, clipboard copy, and atomic CSV export.
- Stop/restart scans while retaining completed results.
- Follow system application colors, light/dark appearance, and accent colors
  exposed by Qt; no Omarchy configuration or Portalview integration.

Enter an address (`192.168.1.1`), inclusive range
(`192.168.1.1-192.168.1.254`), or CIDR (`192.168.1.0/24`). CIDR excludes the
network/broadcast addresses except for /31 and /32. Local /24 selects the first
active non-loopback IPv4 adapter; check the range when using VPNs or virtual
adapters. Host concurrency is 1–64 (default 32); worker execution is additionally
bounded by Qt's thread pool. Timeout is 200–5,000 ms per probe.

**Live** means ICMP responded or a tested TCP port opened. **No reply** can mean a
firewall blocks discovery. **Error** means an operational probe error occurred
without TCP establishing liveness. **Cancelled** rows were unfinished when stopped.
MAC addresses are best effort on the local network; routed devices usually have
no available MAC. Reverse DNS names may be absent.

Common ports: 21, 22, 23, 25, 53, 80, 110, 139, 143, 443, 445, 587, 631, 993, 995,
1433, 3306, 3389, 5432, 5900, 8000, 8080, 8443. These are TCP connection checks,
not service identification or UDP scans.

Right-click a row to copy its IP/details; double-click to copy its IP. Ctrl+F
focuses search; Escape stops scanning. Export includes every row regardless of
view filters. The display keeps Lantern's custom layout while using system colors.

## Build on Windows

Install Visual Studio 2022 Build Tools with Desktop development with C++ and a
Windows SDK, CMake 3.24+, Ninja, and the matching Qt 6.8+ MSVC 2022 x64 kit
(including Qt Quick, Quick Controls, Network, Concurrent, and Test).
From an x64 Native Tools command prompt with Qt's `bin` directory on PATH:

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.8.3/msvc2022_64
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
./packaging/windows/deploy.ps1
```

The CI workflow also bundles Qt license materials and tests the deployed app
with Qt paths removed from the environment. See the workflow for distribution
steps. Use a matching compiler/Qt architecture; do not mix MinGW and MSVC kits.

## Linux development checks

The original Linux backend remains available for development. Install Qt 6.8+
Base/Declarative, CMake, Ninja, a C++20 compiler, and iputils:

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Tests use injected structured probe results rather than POSIX fake executables;
Windows CI additionally exercises the native ICMP backend. An offscreen startup
check and populated UI/system-palette checks are included.

## Releases

Pushes to `main` build/test/package development artifacts. Pushing a version tag
such as `v0.1.0` builds and publishes a GitHub Release with the portable Windows ZIP
and `SHA256SUMS.txt`, after all tests and the packaged startup check pass.
The tag must match `project(... VERSION ...)` in CMakeLists.txt. That version also
sets the EXE's Windows version metadata. Release uploads are staged as a draft;
published assets are verified on retries and are never overwritten.

To prepare a later release, update the CMake project version, push the reviewed
commit, then create and push the matching version tag. Creating a tag is the
publication step. No release is published by ordinary branch pushes or manual
build runs.

## Project and license

Forked from upstream commit `680b1642c926b646572d8d311d4fb4b527a76f29`.
See [Windows port notes](WINDOWS-PORT.md) for architecture and remaining manual checks.
Inherited Arch packaging and release scripts have been removed.

Copyright © 2026 [seth-reee](https://github.com/seth-reee).
Application source is available under the [MIT License](LICENSE).
Qt libraries have their own licenses; see the notices distributed with builds.
