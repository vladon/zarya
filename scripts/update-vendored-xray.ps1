param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^v[0-9]')]
    [string]$Tag,

    [Parameter(Mandatory = $true)]
    [ValidatePattern('^[0-9a-fA-F]{40}$')]
    [string]$ExpectedCommit
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$thirdPartyRoot = Join-Path $repositoryRoot 'third_party'
$destination = Join-Path $thirdPartyRoot 'xray-core'
$manifestPath = Join-Path $thirdPartyRoot 'xray-core.zarya.json'
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("zarya-xray-update-" + [guid]::NewGuid().ToString('N'))
$checkout = Join-Path $temporaryRoot 'checkout'
$staged = Join-Path $temporaryRoot 'staged'
$backup = Join-Path $temporaryRoot 'previous'

try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    git clone --quiet --filter=blob:none --no-checkout https://github.com/XTLS/Xray-core.git $checkout
    git -C $checkout fetch --quiet --depth 1 origin "refs/tags/$Tag:refs/tags/$Tag"
    git -C $checkout checkout --quiet --detach $Tag

    $actualCommit = (git -C $checkout rev-parse HEAD).Trim().ToLowerInvariant()
    if ($actualCommit -ne $ExpectedCommit.ToLowerInvariant()) {
        throw "Tag $Tag resolved to $actualCommit, expected $ExpectedCommit."
    }

    New-Item -ItemType Directory -Path $staged | Out-Null
    Get-ChildItem -LiteralPath $checkout -Force |
        Where-Object { $_.Name -notin @('.git', '.gitmodules') } |
        ForEach-Object { Copy-Item -LiteralPath $_.FullName -Destination $staged -Recurse }

    if (-not (Test-Path -LiteralPath (Join-Path $staged 'LICENSE'))) {
        throw 'The staged checkout has no LICENSE file.'
    }
    if (Test-Path -LiteralPath (Join-Path $staged '.git')) {
        throw 'The staged checkout unexpectedly contains .git.'
    }

    if (Test-Path -LiteralPath $destination) {
        Move-Item -LiteralPath $destination -Destination $backup
    }
    Move-Item -LiteralPath $staged -Destination $destination

    $goDirective = (Select-String -LiteralPath (Join-Path $destination 'go.mod') -Pattern '^go\s+(.+)$').Matches.Groups[1].Value
    $moduleVersion = $Tag -replace '^v([0-9]+)\.([0-9]+)\.([0-9]+)$', 'v1.$1$2$3.0'
    $manifest = [ordered]@{
        name = 'Xray-core'
        repository = 'https://github.com/XTLS/Xray-core.git'
        tag = $Tag
        moduleVersion = $moduleVersion
        commit = $actualCommit
        goVersion = "$goDirective.x"
        license = 'MPL-2.0'
        sourceDirectory = 'third_party/xray-core'
        updateCommand = ".\scripts\update-vendored-xray.ps1 -Tag $Tag -ExpectedCommit $actualCommit"
    }
    $manifest | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Write-Host "Vendored Xray-core updated to $Tag ($actualCommit)."
} catch {
    if (-not (Test-Path -LiteralPath $destination) -and (Test-Path -LiteralPath $backup)) {
        Move-Item -LiteralPath $backup -Destination $destination
    }
    throw
} finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}
