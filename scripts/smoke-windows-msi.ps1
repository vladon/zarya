#Requires -Version 5.1
param(
    [string]$MsiPath = "",
    [string]$DistDir = "dist",
    [switch]$Manual,
    [switch]$RunInstallTest,
    [switch]$RequireMsi
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Meta = & python -c "import sys; sys.path.insert(0, r'$Root\scripts'); from release_common import read_cmake_version; import json; print(json.dumps(read_cmake_version()))"
$Version = ($Meta | ConvertFrom-Json).version

$Dist = if ([System.IO.Path]::IsPathRooted($DistDir)) { $DistDir } else { Join-Path $Root $DistDir }
if ([string]::IsNullOrWhiteSpace($MsiPath)) {
    $MsiPath = Join-Path $Dist "Zarya-$Version-windows-x64-installer-poc.msi"
}

$errors = @()

$msiPresent = Test-Path $MsiPath
if ($RequireMsi -and -not $msiPresent) {
    $errors += "MSI not found: $MsiPath"
}

$shaPath = "$MsiPath.sha256"
if ($msiPresent) {
    if (-not (Test-Path $shaPath)) {
        $errors += "MSI checksum sidecar missing: $shaPath"
    }
} elseif (-not $RequireMsi) {
    Write-Host "MSI artifact not built yet (skeleton checks only). Build with scripts\package-windows-msi.ps1"
}

$requiredWix = @(
    "packaging\windows\wix\Product.wxs",
    "packaging\windows\wix\Directories.wxs",
    "packaging\windows\wix\Registry.wxs",
    "packaging\windows\wix\Shortcuts.wxs",
    "scripts\package-windows-msi.ps1",
    "scripts\generate-wix-components.py",
    "docs\installer\windows-msi-poc.md"
)
foreach ($rel in $requiredWix) {
    if (-not (Test-Path (Join-Path $Root $rel))) {
        $errors += "missing source file: $rel"
    }
}

if ($Manual -or $RunInstallTest) {
    Write-Host "Manual MSI install test is not run automatically in CI."
    Write-Host "Install: msiexec /i `"$MsiPath`""
    Write-Host "Uninstall: msiexec /x `"$MsiPath`""
    Write-Host "Verify: `"${env:ProgramFiles}\Zarya\Zarya.exe`" --version"
    Write-Host "Helper service should be absent unless INSTALLHELPERSERVICE=1 was used."
}

if ($errors.Count -gt 0) {
    Write-Error ($errors -join "`n")
    exit 1
}

Write-Host "MSI smoke checks passed for $MsiPath"
exit 0
