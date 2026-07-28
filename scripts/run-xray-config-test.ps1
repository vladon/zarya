# Run zarya_xray_config_test (MSVC). Does not reconfigure an existing build tree.
param(
    [switch]$Configure
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Cache = Join-Path $RepoRoot "build\CMakeCache.txt"

if (-not (Test-Path $Cache)) {
    if ($Configure) {
        & (Join-Path $PSScriptRoot "configure-msvc2026.ps1")
    } else {
        Write-Error "No build tree. Run .\scripts\configure-msvc2026.ps1 first."
    }
}

$Config = "Release"

Push-Location $RepoRoot
try {
    cmake --build build --config $Config --target zarya_xray_config_test -j 8
    $testExe = Join-Path $RepoRoot "build\$Config\zarya_xray_config_test.exe"
    if (-not (Test-Path $testExe)) {
        Write-Error "Test binary not found: $testExe"
    }

    & $testExe
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
