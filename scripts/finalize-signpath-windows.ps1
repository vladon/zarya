#Requires -Version 5.1
param(
    [Parameter(Mandatory = $true)]
    [string]$StagingDir,
    [string]$OutputDir = "dist",
    [string]$ExpectedPublisher = "SignPath Foundation"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Meta = & python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from release_common import read_cmake_version; import json; print(json.dumps(read_cmake_version()))"
$VersionInfo = $Meta | ConvertFrom-Json
$Version = $VersionInfo.version
$ArtifactBase = "Zarya-$Version-windows-x64-portable"

$Staging = if ([System.IO.Path]::IsPathRooted($StagingDir)) {
    [System.IO.Path]::GetFullPath($StagingDir)
} else {
    [System.IO.Path]::GetFullPath((Join-Path $Root $StagingDir))
}
if (-not (Test-Path -LiteralPath $Staging -PathType Container)) {
    throw "Signed staging directory not found: $Staging"
}
if ((Split-Path -Leaf $Staging) -ne $ArtifactBase) {
    throw "Signed staging directory must be named $ArtifactBase"
}

$RequiredExecutables = @("Zarya.exe", "zarya-helper.exe", "zarya-updater.exe")
$Signtool = Get-Command signtool -ErrorAction SilentlyContinue
if (-not $Signtool) {
    $SdkSigntool = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe"
    ) | ForEach-Object { Get-Item $_ -ErrorAction SilentlyContinue } |
        Sort-Object FullName -Descending |
        Select-Object -First 1
    if ($SdkSigntool) {
        $Signtool = @{ Source = $SdkSigntool.FullName }
    }
}
if (-not $Signtool) {
    throw "signtool.exe not found; refusing to finalize an unverified signed artifact"
}
$SigntoolExe = if ($Signtool.Source) { $Signtool.Source } else { $Signtool.Path }

foreach ($Name in $RequiredExecutables) {
    $Path = Join-Path $Staging $Name
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "Signed artifact is missing $Name"
    }
    & $SigntoolExe verify /pa /all /tw /v $Path
    if ($LASTEXITCODE -ne 0) {
        throw "Authenticode verification failed for $Name"
    }
    $Signature = Get-AuthenticodeSignature -LiteralPath $Path
    if ($Signature.Status -ne "Valid" -or -not $Signature.SignerCertificate) {
        throw "Windows does not report a valid Authenticode signature for $Name"
    }
    if ($Signature.SignerCertificate.Subject -notlike "*$ExpectedPublisher*") {
        throw "Unexpected Authenticode publisher for ${Name}: $($Signature.SignerCertificate.Subject)"
    }
}

python -c @"
import sys
from pathlib import Path
sys.path.insert(0, r'$Root\scripts')
from release_common import (
    build_signing_manifest,
    refresh_manifest_checksums,
    update_manifest_signing,
    write_build_integrity,
)

staging = Path(r'$Staging')
signing = build_signing_manifest(
    signed=True,
    signature_type='windows-authenticode-signpath-foundation',
    timestamped=True,
    verification={'windowsAuthenticode': 'valid'},
)
if update_manifest_signing(staging, signing) is None:
    raise SystemExit('release-manifest.json is missing from signed staging')
write_build_integrity(staging, signing)
refresh_manifest_checksums(staging)
"@
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

$DistDir = if ([System.IO.Path]::IsPathRooted($OutputDir)) {
    $OutputDir
} else {
    Join-Path $Root $OutputDir
}
New-Item -ItemType Directory -Path $DistDir -Force | Out-Null
$ZipPath = Join-Path $DistDir "$ArtifactBase.zip"
if (Test-Path -LiteralPath $ZipPath) {
    Remove-Item -LiteralPath $ZipPath -Force
}
Compress-Archive -Path $Staging -DestinationPath $ZipPath
python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from pathlib import Path; from release_common import write_checksum_sidecars; write_checksum_sidecars(Path(r'$DistDir'), Path(r'$ZipPath'))"
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

& python (Join-Path $Root "scripts\smoke-package.py") --artifact $ZipPath
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
& python (Join-Path $Root "scripts\verify-release-artifacts.py") `
    --artifact $ZipPath `
    --expected-version $Version `
    --stable-release `
    --require-checksum `
    --require-signed
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Finalized signed artifact $ZipPath"
