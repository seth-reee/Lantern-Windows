param(
    [string]$BuildDir = 'build',
    [string]$Destination = 'dist/Lantern-Windows'
)
$ErrorActionPreference = 'Stop'
$Destination = [IO.Path]::GetFullPath($Destination)
cmake --install $BuildDir --prefix $Destination
if ($LASTEXITCODE -ne 0) { throw 'Application install failed' }
windeployqt --release --qmldir "$PSScriptRoot/../../qml" --no-compiler-runtime "$Destination/lantern.exe"
if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed' }

# Bundle the redistributable CRT DLLs beside the EXE, rather than requiring users
# to install vc_redist.x64.exe. Windows 11 supplies the Universal CRT/system DLLs.
if (!$env:VCToolsRedistDir) { throw 'Run from an x64 Visual Studio developer environment' }
$runtime = Join-Path $env:VCToolsRedistDir 'x64/Microsoft.VC143.CRT'
if (!(Test-Path $runtime)) { throw "MSVC x64 redistributable directory missing: $runtime" }
Copy-Item "$runtime/*.dll" $Destination
foreach ($name in @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    if (!(Test-Path (Join-Path $Destination $name))) { throw "Missing runtime DLL: $name" }
}

# Hosted runners already have MSVC installed. Check imports explicitly so a
# successful startup there cannot conceal missing Qt or compiler runtime DLLs.
Get-ChildItem $Destination -Recurse -File | Where-Object { $_.Extension -in '.exe', '.dll' } | ForEach-Object {
    $binary = $_.FullName
    $imports = & dumpbin /nologo /dependents $binary
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect dependencies: $binary" }
    foreach ($line in $imports) {
        if ($line -match '^\s+((?:Qt6|MSVCP|VCRUNTIME|CONCRT)[^\s]*\.dll)\s*$') {
            $dependency = $Matches[1]
            if (!(Test-Path (Join-Path $Destination $dependency))) {
                throw "Missing app-local dependency $dependency required by $binary"
            }
        }
    }
}

python "$PSScriptRoot/collect-qt-notices.py" "$Destination/qt-licenses"
if ($LASTEXITCODE -ne 0) { throw 'Qt license collection failed' }
Copy-Item "$PSScriptRoot/../../README.md" "$Destination/README.md"
