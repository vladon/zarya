# Build Zarya (adds CMake to PATH for shells where it is not preconfigured).
# Static Qt is the default on Windows; pass -Shared for shared Qt (faster iteration).
# Pass -DesktopAppUi to enable/link lib_ui (reconfigures if the flag changed).
param(
    [string]$Config = "Release",
    [string]$Target = "zarya",
    [switch]$Test,
    [switch]$Shared,
    [switch]$DesktopAppUi,
    [switch]$Force
)

$ErrorActionPreference = "Stop"

$cmakeBin = "C:\Program Files\CMake\bin"
if (-not (Test-Path "$cmakeBin\cmake.exe")) {
    Write-Error "CMake not found at $cmakeBin. Install from https://cmake.org/download/ or Visual Studio."
}

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
if ($Shared) {
    $QtRoot = if ($env:QT_ROOT) { $env:QT_ROOT } else { "C:\Qt" }
    $QtMsvc = "$QtRoot\$QtVersion\msvc2022_64"
} else {
    $QtMsvc = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
}
$QtBin = "$QtMsvc\bin"
$env:Path = "$cmakeBin;$QtBin;$env:Path"

$RepoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $RepoRoot
try {
    $cache = "build\CMakeCache.txt"
    $needConfigure = $Force -or -not (Test-Path $cache)
    if (-not $needConfigure -and (Test-Path $cache)) {
        $wasDesktopUi = Select-String -Path $cache -Pattern "^ZARYA_DESKTOP_APP_UI:BOOL=ON" -Quiet
        if ($DesktopAppUi -and -not $wasDesktopUi) { $needConfigure = $true }
    }
    if ($needConfigure) {
        Write-Host "Running configure-msvc2026.ps1 ..."
        $configureArgs = @()
        if (-not $Shared) { $configureArgs += "-Static" }
        if ($DesktopAppUi) { $configureArgs += "-DesktopAppUi" }
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
