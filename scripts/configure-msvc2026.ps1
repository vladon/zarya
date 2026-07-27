# Configure Zarya with Visual Studio 2026 (MSVC) and Qt.
# Use -Static for a fully static Release binary (requires scripts/build-qt-static-msvc2026.ps1).
# Desktop App UI (lib_ui, GPLv3+) is ON by default for -Static; use -NoDesktopAppUi to disable.
# Use -DesktopAppUi to force-enable on shared Qt configures.
param(
    [switch]$Static,
    [switch]$Force,
    [switch]$DesktopAppUi,
    [switch]$NoDesktopAppUi
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

# Desktop App UI ON by default for static builds unless -NoDesktopAppUi
$useDesktopAppUi = if ($NoDesktopAppUi) { $false }
                   elseif ($DesktopAppUi) { $true }
                   elseif ($Static) { $true }
                   else { $false }

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
            $wasDesktopUi = Select-String -Path $cache -Pattern "^ZARYA_DESKTOP_APP_UI:BOOL=ON" -Quiet
            $wantDesktopUi = [bool]$useDesktopAppUi
            if ($gen -and $gen -notmatch "Visual Studio 18 2026") { $remove = $true }
            if ($wasStatic -ne $wantStatic) { $remove = $true }
            if ($wasDesktopUi -ne $wantDesktopUi) { $remove = $true }
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
    if ($useDesktopAppUi) {
        $cmakeArgs += "-DZARYA_DESKTOP_APP_UI=ON"
        if (Test-Path "C:\Program Files\OpenSSL-Win64") {
            $cmakeArgs += "-DOPENSSL_ROOT_DIR=C:/Program Files/OpenSSL-Win64"
        } elseif (Test-Path (Join-Path $RepoRoot "third_party\desktop-app\openssl-win64")) {
            $cmakeArgs += "-DOPENSSL_ROOT_DIR=$RepoRoot/third_party/desktop-app/openssl-win64"
        }
    }

    cmake @cmakeArgs

    Write-Host ""
    $mode = if ($Static) { "static Qt (/MT)" } else { "shared Qt" }
    $ui = if ($useDesktopAppUi) { " + Desktop App UI (GPLv3+)" } else { "" }
    Write-Host "Configured: Visual Studio 2026 + $mode$ui"
    Write-Host "Qt path:   $QtMsvc"
    Write-Host "Build app:  cmake --build build --config $Config --target zarya"
    Write-Host "Run app:    .\build\$Config\zarya.exe"
} finally {
    Pop-Location
}
