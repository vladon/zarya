#Requires -Version 5.1
param(
    [string]$Configuration = "Release",
    [string]$BuildDir = "build",
    [string]$OutputDir = "dist",
    [switch]$SkipBuild,
    [switch]$Sign,
    [switch]$SkipSigning,
    [string]$SigningThumbprint = "",
    [string]$TimestampUrl = "http://timestamp.digicert.com",
    [switch]$IncludeHelperServiceFeature
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Meta = & python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from release_common import read_cmake_version; import json; print(json.dumps(read_cmake_version()))"
$VersionInfo = $Meta | ConvertFrom-Json
$Version = $VersionInfo.version
$MsiVersion = if ($Version -match '^(\d+)\.(\d+)\.(\d+)') { "$($Matches[1]).$($Matches[2]).$($Matches[3]).0" } else { "0.34.0.0" }

$DoSign = $Sign.IsPresent
if ($SkipSigning.IsPresent) {
    $DoSign = $false
}

$DistDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) { $OutputDir } else { Join-Path $Root $OutputDir }
$BuildRoot = Join-Path $Root $BuildDir
$BuildOutput = Join-Path $BuildRoot $Configuration
$ArtifactBase = "Zarya-$Version-windows-x64-installer-poc"
$Staging = Join-Path $DistDir "$ArtifactBase-staging"
$MsiPath = Join-Path $DistDir "$ArtifactBase.msi"
$WixDir = Join-Path $Root "packaging\windows\wix"
$GeneratedDir = Join-Path $WixDir "generated"

if (-not $SkipBuild) {
    Write-Host "Configuring and building ($Configuration)..."
    if (-not (Test-Path (Join-Path $BuildRoot "CMakeCache.txt"))) {
        & (Join-Path $Root "scripts\configure-msvc2026.ps1") -BuildDir $BuildDir
    }
    cmake --build $BuildRoot --config $Configuration --target zarya_lrelease zarya zarya-helper zarya-updater
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

$UpdaterExe = Join-Path $BuildOutput "zarya-updater.exe"
if (-not (Test-Path $UpdaterExe)) {
    throw "zarya-updater.exe not found under $BuildOutput"
}

if (Test-Path $Staging) { Remove-Item -Recurse -Force $Staging }
New-Item -ItemType Directory -Path $Staging | Out-Null

Copy-Item $GuiExe (Join-Path $Staging "Zarya.exe")
Copy-Item $HelperExe (Join-Path $Staging "zarya-helper.exe")
Copy-Item $UpdaterExe (Join-Path $Staging "zarya-updater.exe")
New-Item -ItemType File -Path (Join-Path $Staging ".zarya-installed") | Out-Null

python -c @"
import sys
from pathlib import Path
sys.path.insert(0, r'$Root\scripts')
from release_common import (
    copy_top_level_legal_files,
    copy_docs,
    copy_public_beta_docs,
    copy_installer_docs,
    copy_updater_docs,
    copy_stable_docs,
    copy_rc_docs,
    copy_stable_release_docs,
    copy_service_packaging_templates,
    copy_translations,
    create_placeholder_layout,
    write_release_manifest,
    verify_clean_staging,
    write_build_integrity,
)

staging = Path(r'$Staging')
build_translations = Path(r'$BuildRoot') / 'translations'
copy_top_level_legal_files(staging)
copy_docs(staging)
copy_public_beta_docs(staging)
copy_installer_docs(staging)
copy_updater_docs(staging)
copy_stable_docs(staging)
copy_rc_docs(staging)
copy_stable_release_docs(staging)
copy_service_packaging_templates(staging)
copy_translations(staging, build_translations)
create_placeholder_layout(staging)
write_release_manifest(
    staging,
    platform='windows',
    architecture='x64',
    portable=False,
    gui_artifact='Zarya.exe',
    helper_artifact='zarya-helper.exe',
    updater_artifact='zarya-updater.exe',
    artifact_type='windows-msi-poc',
    installation_mode='installed',
    helper_service={'included': True, 'installedByDefault': False},
)
write_build_integrity(staging)
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

if ($DoSign) {
    $Thumb = $SigningThumbprint
    if ([string]::IsNullOrWhiteSpace($Thumb)) {
        $Thumb = $env:ZARYA_WINDOWS_CERT_THUMBPRINT
    }
    if ([string]::IsNullOrWhiteSpace($Thumb)) {
        throw "Signing requested but no certificate thumbprint (-SigningThumbprint or ZARYA_WINDOWS_CERT_THUMBPRINT)"
    }
    $SignScript = Join-Path $Root "scripts\sign-windows.ps1"
    & $SignScript -File (Join-Path $Staging "Zarya.exe") -CertificateThumbprint $Thumb -TimestampUrl $TimestampUrl -Verify
    & $SignScript -File (Join-Path $Staging "zarya-helper.exe") -CertificateThumbprint $Thumb -TimestampUrl $TimestampUrl -Verify
}

python (Join-Path $Root "scripts\generate-wix-components.py") --staging $Staging --output-dir $GeneratedDir

$Wix = Get-Command wix -ErrorAction SilentlyContinue
if (-not $Wix) {
    throw "WiX Toolset (wix) not found on PATH. Install WiX 4.x to build the MSI PoC."
}

$WixSources = @(
    (Join-Path $WixDir "Product.wxs"),
    (Join-Path $WixDir "Directories.wxs"),
    (Join-Path $WixDir "Registry.wxs"),
    (Join-Path $WixDir "Shortcuts.wxs"),
    (Join-Path $GeneratedDir "GeneratedDirectories.wxs"),
    (Join-Path $GeneratedDir "GeneratedComponents.wxs")
)

New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
if (Test-Path $MsiPath) { Remove-Item $MsiPath }

$WixArgs = @(
    "build",
    "-arch", "x64",
    "-d", "ProductVersion=$MsiVersion",
    "-bindpath", "staging=$Staging",
    "-o", $MsiPath
) + $WixSources

Write-Host "Building MSI with WiX..."
& wix @WixArgs

python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path(r'$DistDir'), Path(r'$MsiPath'))"

Write-Host "Created $MsiPath"
Write-Host "Portable ZIP remains the recommended beta distribution. Build with scripts\package-windows.ps1"

if ($IncludeHelperServiceFeature) {
    Write-Host "To install helper service: msiexec /i `"$MsiPath`" INSTALLHELPERSERVICE=1"
}
