# Build Zarya (adds CMake to PATH for shells where it is not preconfigured).
param(
    [string]$Config = "Release",
    [string]$Target = "zarya",
    [switch]$Test
)

$ErrorActionPreference = "Stop"

$cmakeBin = "C:\Program Files\CMake\bin"
if (-not (Test-Path "$cmakeBin\cmake.exe")) {
    Write-Error "CMake not found at $cmakeBin. Install from https://cmake.org/download/ or Visual Studio."
}

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$QtRoot = if ($env:QT_ROOT) { $env:QT_ROOT } else { "C:\Qt" }
$QtBin = "$QtRoot\$QtVersion\msvc2022_64\bin"
$env:Path = "$cmakeBin;$QtBin;$env:Path"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $RepoRoot
try {
    if (-not (Test-Path "build\CMakeCache.txt")) {
        Write-Host "No build tree — running configure-msvc2026.ps1 ..."
        & "$PSScriptRoot\configure-msvc2026.ps1"
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
