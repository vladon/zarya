# Run zarya_xray_config_test using Visual Studio 2026 (MSVC) + Qt msvc2022_64.
$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path -Parent $PSScriptRoot
& (Join-Path $PSScriptRoot "configure-msvc2026.ps1")

$Config = "Release"
Push-Location $RepoRoot
try {
    cmake --build build --config $Config --target zarya_xray_config_test -j 8
    $testExe = Join-Path $RepoRoot "build\$Config\zarya_xray_config_test.exe"
    if (-not (Test-Path $testExe)) {
        Write-Error "Test binary not found: $testExe"
    }

    $QtMsvc = if ($env:QT_MSVC_DIR) { $env:QT_MSVC_DIR } else { "C:\Qt\6.8.3\msvc2022_64" }
    $env:Path = "$QtMsvc\bin;$env:Path"
    & $testExe
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
