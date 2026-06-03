#Requires -Version 5.1
param(
    [string]$BuildDir = "build",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Version = "0.12.0"
$Staging = Join-Path $Root "dist\Zarya-$Version-windows-x64-portable"
$BuildOutput = Join-Path $Root "$BuildDir\$Config"

Write-Host "Building zarya ($Config)..."
cmake --build (Join-Path $Root $BuildDir) --config $Config --target zarya

$ExeName = "zarya.exe"
$ExePath = Join-Path $BuildOutput $ExeName
if (-not (Test-Path $ExePath)) {
    throw "Executable not found: $ExePath"
}

if (Test-Path $Staging) {
    Remove-Item -Recurse -Force $Staging
}
New-Item -ItemType Directory -Path $Staging | Out-Null

Copy-Item $ExePath (Join-Path $Staging "Zarya.exe")
New-Item -ItemType File -Path (Join-Path $Staging "portable.flag") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Staging "data") | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Staging "runtime") | Out-Null
$coresDir = Join-Path $Staging "cores\xray"
New-Item -ItemType Directory -Path $coresDir -Force | Out-Null
Copy-Item (Join-Path $Root "packaging\windows\cores-xray-README.txt") (Join-Path $coresDir "README.txt")

$WinDeployQt = Get-Command windeployqt -ErrorAction SilentlyContinue
if ($WinDeployQt) {
    & windeployqt (Join-Path $Staging "Zarya.exe")
} else {
    Write-Warning "windeployqt not found; copy Qt runtime DLLs manually."
}

Copy-Item (Join-Path $Root "README.md") $Staging -ErrorAction SilentlyContinue
Copy-Item (Join-Path $Root "LICENSE") $Staging -ErrorAction SilentlyContinue

$ZipPath = Join-Path $Root "dist\Zarya-$Version-windows-x64-portable.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Compress-Archive -Path $Staging -DestinationPath $ZipPath
Write-Host "Created $ZipPath"
