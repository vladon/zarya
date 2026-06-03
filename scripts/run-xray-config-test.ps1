# Run zarya_xray_config_test with Qt MinGW toolchain (Windows).
$ErrorActionPreference = "Stop"

$QtRoot = if ($env:QT_ROOT) { $env:QT_ROOT } else { "C:\Qt" }
$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$MingwKit = "$QtRoot\$QtVersion\mingw_64"
$MingwTools = "$QtRoot\Tools\mingw1310_64"

if (-not (Test-Path "$MingwKit\bin\qmake.exe")) {
    Write-Error "Qt not found at $MingwKit. Set QT_ROOT / QT_VERSION or install via: python -m aqt install-qt windows desktop $QtVersion win64_mingw -O $QtRoot"
}

$env:Path = "$MingwTools\bin;$MingwKit\bin;C:\Program Files\CMake\bin;$env:Path"
$RepoRoot = Split-Path -Parent $PSScriptRoot

Push-Location $RepoRoot
try {
    if (-not (Test-Path build\CMakeCache.txt)) {
        cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="$MingwKit" -DCMAKE_BUILD_TYPE=Release
    }
    cmake --build build --target zarya_xray_config_test -j 4
    & .\build\zarya_xray_config_test.exe
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
