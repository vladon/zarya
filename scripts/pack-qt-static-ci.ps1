# Pack a static Qt prefix into dist/ for CI restore (7-Zip).
#   .\scripts\pack-qt-static-ci.ps1
#   .\scripts\pack-qt-static-ci.ps1 -Prefix C:\Qt\Static\6.8.3\msvc2022_64
param(
    [string]$Prefix = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
if (-not $Prefix) {
    $Prefix = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
}

$RepoRoot = Split-Path -Parent $PSScriptRoot
if (-not $OutDir) {
    $OutDir = Join-Path $RepoRoot "dist"
}

$ConfigMarker = Join-Path $Prefix "lib\cmake\Qt6\Qt6Config.cmake"
if (-not (Test-Path $ConfigMarker)) {
    Write-Error "Static Qt prefix incomplete (missing $ConfigMarker). Build with .\scripts\build-qt-static-msvc2026.ps1 first."
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$ArchiveName = "qt-static-$QtVersion-msvc2022_64.7z"
$ArchivePath = Join-Path $OutDir $ArchiveName

$SevenZip = @(
    "${env:ProgramFiles}\7-Zip\7z.exe",
    "${env:ProgramFiles(x86)}\7-Zip\7z.exe",
    "C:\ProgramData\chocolatey\bin\7z.exe"
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $SevenZip) {
    Write-Host "Installing 7zip via chocolatey..."
    choco install 7zip -y --no-progress
    $SevenZip = @(
        "${env:ProgramFiles}\7-Zip\7z.exe",
        "C:\ProgramData\chocolatey\bin\7z.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $SevenZip) {
    Write-Error "7z.exe not found after install"
}

if (Test-Path $ArchivePath) {
    Remove-Item -Force $ArchivePath
}

Write-Host "Packing $Prefix -> $ArchivePath"
# Archive contents as msvc2022_64/... so extract into C:\Qt\Static\6.8.3\ yields the kit.
$parent = Split-Path -Parent $Prefix
$leaf = Split-Path -Leaf $Prefix
Push-Location $parent
try {
    & $SevenZip a -t7z -mx=5 -mmt=on $ArchivePath $leaf
    if ($LASTEXITCODE -ne 0) {
        throw "7z failed with exit $LASTEXITCODE"
    }
} finally {
    Pop-Location
}

$size = (Get-Item $ArchivePath).Length
Write-Host ("Created {0} ({1:N1} MB)" -f $ArchivePath, ($size / 1MB))
Write-Output $ArchivePath
