# Windows port notes

Forked 2026-10-19 from Lantern commit
`680b1642c926b646572d8d311d4fb4b527a76f29`.
Target: Windows 11 x64, MSVC 2022, Qt 6.8.3, C++20, CMake/Ninja.

## Implemented

- Native ICMP through IcmpCreateFile/IcmpSendEcho/IcmpCloseHandle, with structured
  reply/error handling and millisecond latency. Windows ping output is never parsed.
- Probe workers use QtConcurrent and own their resources. The GUI stays responsive.
  Stopping destroys completion watchers and cancels queued work through a shared
  token; already running calls finish within their bounded timeout. Late results
  cannot update destroyed model rows or a new scan. Qt's global pool adds a bound
  on executing workers in addition to the scanner's 1–64 active-host limit.
- Windows neighbor MAC lookup uses GetIpNetTable2/FreeMibTable, rejects unresolved
  or invalid entries, and retains local-interface MAC lookup through Qt.
- System application palette with live changes; no Omarchy file polling.
  Semantic red/green indicators retain explicit light/dark variants. The existing
  custom layout remains, rather than reproducing every Windows control shape.
- Portalview handoff removed from UI, API, and tests; no RDP integration planned.
- Windows GUI executable, embedded icon/version resource, platform-aware install
  paths, UTF-8 MSVC sources, and portable temporary screenshot paths.
- Portable scanner fixtures, cancellation/restart regression coverage, native
  Windows loopback test, CSV/Unicode-path, populated UI, and palette-change checks.
- Windows CI builds/tests, deploys Qt/QML/compiler dependencies, checks the packaged
  executable with development paths removed, verifies app-local Qt/MSVC DLL imports,
  and uploads a ZIP and SHA-256 checksum. Microsoft C++ runtime DLLs are bundled
  directly, without requiring a separate runtime installer.
- English-only deployment with the Basic control style; translations, unused
  style plugins/DLLs, IDE type metadata, QML debugging plugins, and optional
  software OpenGL are excluded. Qt notices are retained in one document.
- The trimmed package is tested with development paths removed: populated UI,
  menus, CSV export dialog, and rendered startup frames through both Qt software
  rendering and Direct3D 11/WARP.
- Separate repository with upstream history. Original Linux Lantern is unchanged;
  inherited Arch packaging and publishing automation have been removed.

## Reported VM validation

On 2026-10-19 the user confirmed that the packaged application launched and
scanned successfully in a Windows test VM. The VM OS version, Qt installation
state, and detailed DNS/MAC/port results were not specified.

## Remaining manual acceptance checks

- Confirm launch on a Windows 11 machine without Qt installed.
- Scan a known live LAN host, an ICMP-blocked TCP-only host, and a silent address.
- Check DNS and available MAC values against the local network's actual devices.
  Neighbor lookup is best effort and enumerates the table per completed host;
  profile/cache it if large live scans expose a bottleneck.
- Scan /16, stop/restart/close mid-scan and during DNS; check resource use and UI
  responsiveness. Running native calls can take up to the probe timeout to finish.
- Change Windows light/dark, accent, and contrast settings while the app is open;
  inspect labels, menus, disabled controls, and status colors. Check 100–200% DPI
  and hardware graphics. Qt palette unit tests cannot substitute for these checks.
- Try Unicode/spaced save paths and CSV in the intended spreadsheet application.
- Verify the selected subnet on machines with VPN/Hyper-V/virtual adapters. The
  first active adapter is still used; explicit adapter selection is future work.

The preview is unsigned and portable. Installer/signing and ARM64 are follow-up
work. Version tags matching the CMake project version publish tested ZIP/checksum
assets as GitHub Releases; ordinary branch pushes produce development artifacts.

## Platform references

- https://learn.microsoft.com/en-us/windows/win32/api/icmpapi/nf-icmpapi-icmpsendecho
- https://learn.microsoft.com/en-us/windows/win32/api/netioapi/nf-netioapi-getipnettable2
- https://doc.qt.io/qt-6.8/windows.html
- https://doc.qt.io/qt-6.8/windows-deployment.html
- https://doc.qt.io/qt-6.8/qpalette.html
