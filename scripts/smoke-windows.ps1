#Requires -Version 5.1
param(
    [string]$ArtifactDir = "dist"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Dist = if ([System.IO.Path]::IsPathRooted($ArtifactDir)) { $ArtifactDir } else { Join-Path $Root $ArtifactDir }

$Zip = Get-ChildItem -Path $Dist -Filter "Zarya-*-windows-x64-portable.zip" | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $Zip) {
    throw "No Windows portable ZIP found in $Dist"
}

& python (Join-Path $Root "scripts\smoke-package.py") --artifact $Zip.FullName
