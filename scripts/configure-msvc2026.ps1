# Configure Zarya with Visual Studio 2026 (MSVC) and static Qt only.
# Desktop App Toolkit (lib_ui) is always linked (GPLv3+); needs OpenSSL + third_party/desktop-app submodules.
param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$QtMsvc = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }

if (-not (Test-Path "$QtMsvc\lib\cmake\Qt6\Qt6Config.cmake")) {
    Write-Error @"
Static Qt not found at $QtMsvc.
Build it first (one-time, ~30-90 min):
  .\scripts\build-qt-static-msvc2026.ps1
Or set QT_STATIC_DIR to an existing static prefix.
"@
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
            if ($gen -and $gen -notmatch "Visual Studio 18 2026") { $remove = $true }
            if (-not $wasStatic) { $remove = $true }
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
        "-DCMAKE_PREFIX_PATH=$QtMsvc",
        "-DZARYA_STATIC_QT=ON"
    )
    if (Test-Path "C:\Program Files\OpenSSL-Win64") {
        $cmakeArgs += "-DOPENSSL_ROOT_DIR=C:/Program Files/OpenSSL-Win64"
    } elseif (Test-Path (Join-Path $RepoRoot "third_party\desktop-app\openssl-win64")) {
        $cmakeArgs += "-DOPENSSL_ROOT_DIR=$RepoRoot/third_party/desktop-app/openssl-win64"
    }

    cmake @cmakeArgs

    Write-Host ""
    Write-Host "Configured: Visual Studio 2026 + static Qt (/MT) + Desktop App UI (GPLv3+)"
    Write-Host "Qt path:   $QtMsvc"
    Write-Host "Build app:  cmake --build build --config $Config --target zarya"
    Write-Host "Run app:    .\build\$Config\zarya.exe"
} finally {
    Pop-Location
}
