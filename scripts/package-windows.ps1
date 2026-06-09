#Requires -Version 5.1
param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist",
    [string]$StagingDir = "",
    [switch]$SkipBuild,
    [switch]$SkipTests
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Meta = & python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from release_common import read_cmake_version; import json; print(json.dumps(read_cmake_version()))"
$VersionInfo = $Meta | ConvertFrom-Json
$Version = $VersionInfo.version

$DistDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $Root $OutputDir }
$BuildRoot = Join-Path $Root $BuildDir
$BuildOutput = Join-Path $BuildRoot $Configuration
$ArtifactBase = "Zarya-$Version-windows-x64-portable"
$Staging = if ($StagingDir) { $StagingDir } else { Join-Path $DistDir $ArtifactBase }
$ZipPath = Join-Path $DistDir "$ArtifactBase.zip"

if (-not $SkipBuild) {
    Write-Host "Configuring and building ($Configuration)..."
    if (-not (Test-Path (Join-Path $BuildRoot "CMakeCache.txt"))) {
        & (Join-Path $Root "scripts\configure-msvc2026.ps1") -BuildDir $BuildDir
    }
    cmake --build $BuildRoot --config $Configuration --target zarya_lrelease zarya zarya-helper
}

$GuiExe = Join-Path $BuildOutput "Zarya.exe"
if (-not (Test-Path $GuiExe)) {
    $GuiExe = Join-Path $BuildOutput "zarya.exe"
}
if (-not (Test-Path $GuiExe)) {
    throw "Zarya executable not found under $BuildOutput"
}

$HelperExe = Join-Path $BuildOutput "zarya-helper.exe"
if (-not (Test-Path $HelperExe)) {
    throw "zarya-helper.exe not found under $BuildOutput"
}

if (Test-Path $Staging) { Remove-Item -Recurse -Force $Staging }
New-Item -ItemType Directory -Path $Staging | Out-Null

Copy-Item $GuiExe (Join-Path $Staging "Zarya.exe")
Copy-Item $HelperExe (Join-Path $Staging "zarya-helper.exe")
New-Item -ItemType File -Path (Join-Path $Staging "portable.flag") | Out-Null

python -c @"
import sys
from pathlib import Path
sys.path.insert(0, r'$Root\scripts')
from release_common import (
    copy_top_level_legal_files,
    copy_docs,
    copy_translations,
    create_placeholder_layout,
    write_release_manifest,
    verify_clean_staging,
    write_checksum_sidecars,
)

staging = Path(r'$Staging')
build_translations = Path(r'$BuildRoot') / 'translations'
copy_top_level_legal_files(staging)
copy_docs(staging)
copy_translations(staging, build_translations)
create_placeholder_layout(staging)
write_release_manifest(
    staging,
    platform='windows',
    architecture='x64',
    portable=True,
    gui_artifact='Zarya.exe',
    helper_artifact='zarya-helper.exe',
)
errors = verify_clean_staging(staging)
if errors:
    raise SystemExit('\n'.join(errors))
"@

$WinDeployQt = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($WinDeployQt) {
    & windeployqt --release --no-translations (Join-Path $Staging "Zarya.exe")
} else {
    Write-Warning "windeployqt not found; static Qt or manual Qt deployment assumed."
}

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Compress-Archive -Path $Staging -DestinationPath $ZipPath
python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path(r'$DistDir'), Path(r'$ZipPath'))"
Write-Host "Created $ZipPath"

if (-not $SkipTests) {
    & python (Join-Path $Root "scripts\smoke-package.py") --artifact $ZipPath
}
