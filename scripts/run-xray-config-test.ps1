# Run zarya_xray_config_test (MSVC). Does not reconfigure an existing build tree.
param(
    [switch]$ConfigureShared
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Cache = Join-Path $RepoRoot "build\CMakeCache.txt"

if (-not (Test-Path $Cache)) {
    if ($ConfigureShared) {
        & (Join-Path $PSScriptRoot "configure-msvc2026.ps1")
    } else {
        Write-Error "No build tree. Run configure-msvc2026.ps1 or configure-msvc2026.ps1 -Static first."
    }
}

$Config = "Release"
$IsStatic = Select-String -Path $Cache -Pattern "^ZARYA_STATIC_QT:BOOL=ON" -Quiet

Push-Location $RepoRoot
try {
    cmake --build build --config $Config --target zarya_xray_config_test -j 8
    $testExe = Join-Path $RepoRoot "build\$Config\zarya_xray_config_test.exe"
    if (-not (Test-Path $testExe)) {
        Write-Error "Test binary not found: $testExe"
    }

    if (-not $IsStatic) {
        $QtMsvc = if ($env:QT_MSVC_DIR) { $env:QT_MSVC_DIR } else { "C:\Qt\6.8.3\msvc2022_64" }
        $env:Path = "$QtMsvc\bin;$env:Path"
    }

    & $testExe
    exit $LASTEXITCODE
} finally {
    Pop-Location
}
