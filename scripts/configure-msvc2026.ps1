# Configure Zarya with Visual Studio 2026 (MSVC) and Qt msvc2022_64 kit.
# Qt ships MSVC 2022 binaries; they are compatible with the VS 2026 toolset.
$ErrorActionPreference = "Stop"

$QtRoot = if ($env:QT_ROOT) { $env:QT_ROOT } else { "C:\Qt" }
$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$QtMsvc = "$QtRoot\$QtVersion\msvc2022_64"

if (-not (Test-Path "$QtMsvc\bin\qmake.exe")) {
    Write-Error @"
Qt MSVC kit not found at $QtMsvc.
Install with:
  python -m aqt install-qt windows desktop $QtVersion win64_msvc2022_64 -O $QtRoot
"@
}

$env:QT_MSVC_DIR = $QtMsvc
$env:Path = "C:\Program Files\CMake\bin;$env:Path"

$RepoRoot = Split-Path -Parent $PSScriptRoot
$BuildDir = Join-Path $RepoRoot "build"
$Config = if ($args -contains "Debug") { "Debug" } else { "Release" }

Push-Location $RepoRoot
try {
    if (Test-Path $BuildDir) {
        $cache = Join-Path $BuildDir "CMakeCache.txt"
        if (Test-Path $cache) {
            $gen = Select-String -Path $cache -Pattern "^CMAKE_GENERATOR:" | ForEach-Object { $_.Line -replace '^CMAKE_GENERATOR:INTERNAL=', '' }
            if ($gen -and $gen -notmatch "Visual Studio 18 2026") {
                Write-Host "Removing build tree (was configured with: $gen)"
                Remove-Item -Recurse -Force $BuildDir
            }
        }
    }

    cmake -S . -B build -G "Visual Studio 18 2026" `
        -DCMAKE_PREFIX_PATH="$QtMsvc"

    Write-Host ""
    Write-Host "Configured for Visual Studio 2026 + Qt at $QtMsvc"
    Write-Host "Build app:     cmake --build build --config $Config --target zarya"
    Write-Host "Build test:    cmake --build build --config $Config --target zarya_xray_config_test"
    Write-Host "Run test:      .\build\$Config\zarya_xray_config_test.exe"
} finally {
    Pop-Location
}
