[CmdletBinding()]
param(
    [string] $ToolRoot
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent (Split-Path -Parent $projectDir)
if (-not $ToolRoot) {
    $ToolRoot = Join-Path $repoDir 'build\tools\lcl-toolchain'
}
$ToolRoot = [IO.Path]::GetFullPath($ToolRoot)
$downloadDir = Join-Path $ToolRoot 'downloads'
$lazarusDir = Join-Path $ToolRoot 'lazarus'
$winlibsDir = Join-Path $ToolRoot 'winlibs'
$lazarusInstaller = Join-Path $downloadDir `
    'lazarus-4.8-fpc-3.2.2-win64.exe'
$winlibsArchive = Join-Path $downloadDir `
    'winlibs-x86_64-posix-seh-gcc-16.1.0-mingw-w64ucrt-14.0.0-r2.zip'

$lazarusUrl = 'https://sourceforge.net/projects/lazarus/files/Lazarus%20Windows%2064%20bits/Lazarus%204.8/lazarus-4.8-fpc-3.2.2-win64.exe/download'
$lazarusSha256 = 'ed25ee171d55e23cf14e0633159fdd2325efba56e186f8ab817ad3bf97d267d7'
$winlibsUrl = 'https://github.com/brechtsanders/winlibs_mingw/releases/download/16.1.0posix-14.0.0-ucrt-r2/winlibs-x86_64-posix-seh-gcc-16.1.0-mingw-w64ucrt-14.0.0-r2.zip'
$winlibsSha256 = '78eff1e2e804b6a6320c713f084b8f820c662104a24cea6a3bfcab82032bdd60'

function Assert-ChildPath {
    param([Parameter(Mandatory = $true)][string] $Path)
    $fullRoot = $ToolRoot.TrimEnd('\')
    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($fullRoot + '\',
        [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing a toolchain path outside $fullRoot`: $fullPath"
    }
    return $fullPath
}

function Get-VerifiedDownload {
    param(
        [Parameter(Mandatory = $true)][string] $Url,
        [Parameter(Mandatory = $true)][string] $Destination,
        [Parameter(Mandatory = $true)][string] $Sha256
    )
    $Destination = Assert-ChildPath $Destination
    if (Test-Path -LiteralPath $Destination) {
        $actual = (Get-FileHash -LiteralPath $Destination `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($actual -eq $Sha256) {
            return
        }
        throw "Cached download hash mismatch: $Destination"
    }
    Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    $actual = (Get-FileHash -LiteralPath $Destination `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Sha256) {
        Remove-Item -LiteralPath $Destination -Force
        throw "Downloaded file hash mismatch: $Destination"
    }
}

New-Item -ItemType Directory -Force -Path $ToolRoot,$downloadDir | Out-Null

$lazbuild = Join-Path $lazarusDir 'lazbuild.exe'
$fpc = Join-Path $lazarusDir 'fpc\3.2.2\bin\x86_64-win64\fpc.exe'
if ((Test-Path -LiteralPath $lazarusDir) -and
    (-not (Test-Path -LiteralPath $lazbuild) -or
     -not (Test-Path -LiteralPath $fpc))) {
    throw "Incomplete Lazarus toolchain at $lazarusDir"
}
if (-not (Test-Path -LiteralPath $lazbuild)) {
    Get-VerifiedDownload -Url $lazarusUrl -Destination $lazarusInstaller `
        -Sha256 $lazarusSha256
    $signature = Get-AuthenticodeSignature -LiteralPath $lazarusInstaller
    if (($signature.Status -ne 'Valid') -or
        ($signature.SignerCertificate.Subject -notlike `
          '*Programming Free Pascal*Lazarus Foundation*')) {
        throw "Lazarus installer signature is not trusted: $($signature.Status)"
    }
    $arguments = @(
        '/SP-', '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART',
        "/DIR=$lazarusDir"
    )
    $installer = Start-Process -FilePath $lazarusInstaller `
        -ArgumentList $arguments -Wait -PassThru -WindowStyle Hidden
    if ($installer.ExitCode -ne 0) {
        throw "Lazarus installer failed with exit code $($installer.ExitCode)"
    }
}

$gcc = Join-Path $winlibsDir `
    'mingw64\bin\x86_64-w64-mingw32-gcc.exe'
if ((Test-Path -LiteralPath $winlibsDir) -and
    (-not (Test-Path -LiteralPath $gcc))) {
    throw "Incomplete WinLibs toolchain at $winlibsDir"
}
if (-not (Test-Path -LiteralPath $gcc)) {
    Get-VerifiedDownload -Url $winlibsUrl -Destination $winlibsArchive `
        -Sha256 $winlibsSha256
    New-Item -ItemType Directory -Force -Path $winlibsDir | Out-Null
    Expand-Archive -LiteralPath $winlibsArchive -DestinationPath $winlibsDir
}

$lazarusVersion = (& $lazbuild --version 2>&1 | Out-String).Trim()
$fpcVersion = (& $fpc -iV 2>&1 | Out-String).Trim()
$gccVersion = (& $gcc -dumpfullversion 2>&1 | Out-String).Trim()
if ($lazarusVersion -notmatch '4\.8') {
    throw "Expected Lazarus 4.8, found: $lazarusVersion"
}
if ($fpcVersion -ne '3.2.2') {
    throw "Expected FPC 3.2.2, found: $fpcVersion"
}
if ($gccVersion -ne '16.1.0') {
    throw "Expected GCC 16.1.0, found: $gccVersion"
}

$envLines = @(
    "LAZARUS_DIR=$lazarusDir",
    "FPC_DIR=$(Split-Path -Parent $fpc)",
    "CC=$gcc"
)
if ($env:GITHUB_ENV) {
    $envLines | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
}

Write-Host "Lazarus: $lazarusVersion"
Write-Host "FPC: $fpcVersion"
Write-Host "MinGW-w64 GCC: $gccVersion"
Write-Host "LAZARUS_DIR=$lazarusDir"
Write-Host "FPC_DIR=$(Split-Path -Parent $fpc)"
Write-Host "CC=$gcc"
