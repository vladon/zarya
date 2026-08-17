[CmdletBinding()]
param(
    [string] $OutputDirectory
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $OutputDirectory) {
    $OutputDirectory = Join-Path $projectDir 'dist'
}
$OutputDirectory = [IO.Path]::GetFullPath($OutputDirectory)
$binDir = Join-Path $projectDir 'bin'
$exe = Join-Path $binDir 'Zarya.exe'
$checksum = Join-Path $binDir 'Zarya.exe.sha256'
$versionFile = Join-Path (Split-Path -Parent (Split-Path -Parent $projectDir)) `
    'cmake\ZaryaVersion.cmake'
$versionText = Get-Content -LiteralPath $versionFile -Raw
if ($versionText -notmatch 'set\(ZARYA_VERSION_STRING "([^"]+)"\)') {
    throw 'Could not read Zarya version from cmake/ZaryaVersion.cmake.'
}
$version = $Matches[1]
$archive = Join-Path $OutputDirectory `
    "Zarya-$version-windows-x64-portable.zip"

if (-not (Test-Path -LiteralPath $exe) -or
    -not (Test-Path -LiteralPath $checksum)) {
    throw 'Zarya.exe and its checksum must be built before packaging.'
}
$binFiles = @(Get-ChildItem -LiteralPath $binDir -File)
if (($binFiles.Count -ne 2) -or
    ($binFiles.Name -notcontains 'Zarya.exe') -or
    ($binFiles.Name -notcontains 'Zarya.exe.sha256')) {
    throw "Release bin must contain exactly Zarya.exe and Zarya.exe.sha256. Found: $($binFiles.Name -join ', ')"
}
$expected = ((Get-Content -LiteralPath $checksum -Raw).Trim() -split '\s+')[0]
$actual = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant()
if ($expected -ne $actual) {
    throw 'Zarya.exe.sha256 does not match Zarya.exe.'
}

New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
if (Test-Path -LiteralPath $archive) {
    Remove-Item -LiteralPath $archive -Force
}
Compress-Archive -LiteralPath $exe,$checksum -DestinationPath $archive

Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [IO.Compression.ZipFile]::OpenRead($archive)
try {
    $entries = @($zip.Entries | ForEach-Object { $_.FullName })
    if (($entries.Count -ne 2) -or
        ($entries -notcontains 'Zarya.exe') -or
        ($entries -notcontains 'Zarya.exe.sha256')) {
        throw "Release ZIP has an invalid file list: $($entries -join ', ')"
    }
}
finally {
    $zip.Dispose()
}

# Outer checksum sidecar consumed by verify-release-artifacts.py
# --require-checksum and by release tooling.
$zipDigest = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
Set-Content -LiteralPath ($archive + '.sha256') -Encoding ascii -NoNewline `
    -Value "$zipDigest $((Split-Path -Leaf $archive))`n"
Write-Host "Packaged: $archive"
