# Build Zarya with static Qt on Windows (adds CMake to PATH when needed).
# Desktop App Toolkit (lib_ui) is always linked (vendored sources + OpenSSL).
param(
    [string]$Config = "Release",
    [string]$Target = "zarya",
    [switch]$Test,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$cmakeBin = "C:\Program Files\CMake\bin"
if (-not (Test-Path "$cmakeBin\cmake.exe")) {
    Write-Error "CMake not found at $cmakeBin. Install from https://cmake.org/download/ or Visual Studio."
}

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$QtMsvc = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
$QtBin = "$QtMsvc\bin"
$env:Path = "$cmakeBin;$QtBin;$env:Path"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $RepoRoot
try {
    $cache = "build\CMakeCache.txt"
    $needConfigure = $Force -or -not (Test-Path $cache)
    if ($needConfigure) {
        Write-Host "Running configure-msvc2026.ps1 ..."
        $configureArgs = @()
        if ($Force) { $configureArgs += "-Force" }
        & "$PSScriptRoot\configure-msvc2026.ps1" @configureArgs
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    cmake --build build --config $Config --target $Target
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    if ($Test) {
        cmake --build build --config $Config --target zarya_stable_hardening_test
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $testExe = Join-Path $RepoRoot "build\$Config\zarya_stable_hardening_test.exe"
        & $testExe
        exit $LASTEXITCODE
    }
} finally {
    Pop-Location
}
