# Configure Zarya with Visual Studio 2026 (MSVC) and Qt.
# Use -Static for a fully static Release binary (requires scripts/build-qt-static-msvc2026.ps1).
param(
    [switch]$Static,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }

if ($Static) {
    $QtMsvc = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
    $StaticFlag = "-DZARYA_STATIC_QT=ON"
} else {
    $QtRoot = if ($env:QT_ROOT) { $env:QT_ROOT } else { "C:\Qt" }
    $QtMsvc = "$QtRoot\$QtVersion\msvc2022_64"
    $StaticFlag = ""
}

if (-not (Test-Path "$QtMsvc\lib\cmake\Qt6\Qt6Config.cmake")) {
    if ($Static) {
        Write-Error @"
Static Qt not found at $QtMsvc.
Build it first (one-time, ~30-90 min):
  .\scripts\build-qt-static-msvc2026.ps1
"@
    } else {
        Write-Error @"
Qt MSVC kit not found at $QtMsvc.
Install with:
  python -m aqt install-qt windows desktop $QtVersion win64_msvc2022_64 -O C:\Qt
"@
    }
}

$env:QT_MSVC_DIR = $QtMsvc
$env:Path = "C:\Program Files\CMake\bin;$env:Path"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build"
$Config = if ($args -contains "Debug") { "Debug" } else { "Release" }

Push-Location $RepoRoot
try {
    if ($Force -or (Test-Path $BuildDir)) {
        $cache = Join-Path $BuildDir "CMakeCache.txt"
        $remove = $Force
        if (-not $remove -and (Test-Path $cache)) {
            $gen = (Select-String -Path $cache -Pattern "^CMAKE_GENERATOR:" | ForEach-Object { $_.Line -replace '^CMAKE_GENERATOR:INTERNAL=', '' })
            $wasStatic = Select-String -Path $cache -Pattern "^ZARYA_STATIC_QT:BOOL=ON" -Quiet
            $wantStatic = [bool]$Static
            if ($gen -and $gen -notmatch "Visual Studio 18 2026") { $remove = $true }
            if ($wasStatic -ne $wantStatic) { $remove = $true }
        }
        if ($remove -and (Test-Path $BuildDir)) {
            Write-Host "Removing build tree for reconfigure..."
            Remove-Item -Recurse -Force $BuildDir
        }
    }

    $cmakeArgs = @(
        "-S", ".",
        "-B", "build",
        "-G", "Visual Studio 18 2026",
        "-DCMAKE_PREFIX_PATH=$QtMsvc"
    )
    if ($StaticFlag) { $cmakeArgs += $StaticFlag }

    cmake @cmakeArgs

    Write-Host ""
    $mode = if ($Static) { "static Qt (/MT)" } else { "shared Qt" }
    Write-Host "Configured: Visual Studio 2026 + $mode"
    Write-Host "Qt path:   $QtMsvc"
    Write-Host "Build app:  cmake --build build --config $Config --target zarya"
    Write-Host "Run app:    .\build\$Config\zarya.exe"
} finally {
    Pop-Location
}
