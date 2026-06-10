#Requires -Version 5.1
param(
    [Parameter(Mandatory = $true)]
    [string]$File,
    [string]$CertificateThumbprint = "",
    [string]$TimestampUrl = "http://timestamp.digicert.com",
    [switch]$Verify,
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $File)) {
    Write-Error "File not found: $File"
    exit 1
}

$Signtool = Get-Command signtool -ErrorAction SilentlyContinue
if (-not $Signtool) {
    $SdkSigntool = @(
        "${env:ProgramFiles(x86)}\Windows Kits\10\bin\*\x64\signtool.exe"
    ) | ForEach-Object { Get-Item $_ -ErrorAction SilentlyContinue } | Sort-Object FullName -Descending | Select-Object -First 1
    if ($SdkSigntool) {
        $Signtool = @{ Source = $SdkSigntool.FullName }
    }
}

if (-not $Signtool) {
    if ($DryRun) {
        Write-Host "DRY RUN: signtool not found; would sign $File"
        exit 0
    }
    Write-Error "signtool.exe not found. Install Windows SDK or run with -DryRun."
    exit 1
}

$SigntoolExe = if ($Signtool.Source) { $Signtool.Source } else { $Signtool.Path }

if ($DryRun -or [string]::IsNullOrWhiteSpace($CertificateThumbprint)) {
    Write-Host "DRY RUN: would sign $File (thumbprint=$CertificateThumbprint)"
    if ($Verify) {
        Write-Host "DRY RUN: would verify $File"
    }
    exit 0
}

& $SigntoolExe sign /fd SHA256 /tr $TimestampUrl /td SHA256 /sha1 $CertificateThumbprint /a $File
if ($LASTEXITCODE -ne 0) {
    Write-Error "signtool sign failed for $File"
    exit $LASTEXITCODE
}

if ($Verify) {
    & $SigntoolExe verify /pa /v $File
    if ($LASTEXITCODE -ne 0) {
        Write-Error "signtool verify failed for $File"
        exit $LASTEXITCODE
    }
}

Write-Host "Signed $File"
exit 0
