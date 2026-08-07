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
$destination = Join-Path $thirdPartyRoot 'sing-box'
$manifestPath = Join-Path $thirdPartyRoot 'sing-box.zarya.json'
$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) ("zarya-singbox-update-" + [guid]::NewGuid().ToString('N'))
$checkout = Join-Path $temporaryRoot 'checkout'
$staged = Join-Path $temporaryRoot 'staged'
$backup = Join-Path $temporaryRoot 'previous'

try {
    New-Item -ItemType Directory -Path $temporaryRoot | Out-Null
    git clone --quiet --filter=blob:none --no-checkout https://github.com/SagerNet/sing-box.git $checkout
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
    if (-not (Test-Path -LiteralPath (Join-Path $staged 'LICENSE'))) { throw 'The staged checkout has no LICENSE file.' }
    if (Test-Path -LiteralPath (Join-Path $staged '.git')) { throw 'The staged checkout unexpectedly contains .git.' }
    if (Test-Path -LiteralPath $destination) { Move-Item -LiteralPath $destination -Destination $backup }
    Move-Item -LiteralPath $staged -Destination $destination
    $upstreamGoVersion = (Select-String -LiteralPath (Join-Path $destination 'go.mod') -Pattern '^go\s+(.+)$').Matches.Groups[1].Value
    [ordered]@{
        name = 'sing-box'; repository = 'https://github.com/SagerNet/sing-box.git'; tag = $Tag
        commit = $actualCommit; upstreamGoVersion = $upstreamGoVersion; bridgeGoVersion = '1.26.5'
        license = 'GPL-3.0-only'; sourceDirectory = 'third_party/sing-box'
        updateCommand = ".\scripts\update-vendored-singbox.ps1 -Tag $Tag -ExpectedCommit $actualCommit"
    } | ConvertTo-Json | Set-Content -LiteralPath $manifestPath -Encoding utf8
    Write-Host "Vendored sing-box updated to $Tag ($actualCommit)."
} catch {
    if (-not (Test-Path -LiteralPath $destination) -and (Test-Path -LiteralPath $backup)) { Move-Item -LiteralPath $backup -Destination $destination }
    throw
} finally {
    if (Test-Path -LiteralPath $temporaryRoot) { Remove-Item -LiteralPath $temporaryRoot -Recurse -Force }
}
