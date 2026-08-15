[CmdletBinding()]
param(
    [string] $ManifestPath,
    [string] $CacheDirectory
)

$ErrorActionPreference = 'Stop'
$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $ManifestPath) {
    $ManifestPath = Join-Path $projectDir 'known-cores.json'
}
if (-not $CacheDirectory) {
    $CacheDirectory = Join-Path $projectDir 'generated\known-cores-ci'
}
$ManifestPath = [IO.Path]::GetFullPath($ManifestPath)
$CacheDirectory = [IO.Path]::GetFullPath($CacheDirectory)
if (-not (Test-Path -LiteralPath $ManifestPath)) {
    throw "Known-core manifest not found: $ManifestPath"
}
New-Item -ItemType Directory -Force -Path $CacheDirectory | Out-Null
$cacheRoot = (Resolve-Path -LiteralPath $CacheDirectory).Path.TrimEnd('\')

function Assert-ChildPath {
    param([Parameter(Mandatory = $true)][string] $Path)
    $fullPath = [IO.Path]::GetFullPath($Path)
    if (-not $fullPath.StartsWith($cacheRoot + '\',
        [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing a known-core path outside $cacheRoot`: $fullPath"
    }
    return $fullPath
}

function Invoke-CoreProbe {
    param(
        [Parameter(Mandatory = $true)][string] $Executable,
        [Parameter(Mandatory = $true)][object[]] $Arguments,
        [int] $TimeoutMilliseconds = 10000
    )
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $Executable
    $startInfo.WorkingDirectory = Split-Path -Parent $Executable
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $Arguments) {
        [void] $startInfo.ArgumentList.Add([string] $argument)
    }
    $process = [Diagnostics.Process]::Start($startInfo)
    try {
        if (-not $process.WaitForExit($TimeoutMilliseconds)) {
            $process.Kill($true)
            throw "Version probe timed out: $Executable"
        }
        $output = ($process.StandardOutput.ReadToEnd() +
            $process.StandardError.ReadToEnd()).Trim()
        if ($process.ExitCode -ne 0) {
            throw "Version probe failed ($($process.ExitCode)): $output"
        }
        return $output
    }
    finally {
        $process.Dispose()
    }
}

function Assert-PeX64 {
    param([Parameter(Mandatory = $true)][string] $Executable)
    $stream = [IO.File]::OpenRead($Executable)
    try {
        $reader = [IO.BinaryReader]::new($stream)
        try {
            if ($reader.ReadUInt16() -ne 0x5a4d) { throw 'Missing MZ header.' }
            $stream.Position = 0x3c
            $peOffset = $reader.ReadInt32()
            $stream.Position = $peOffset
            if ($reader.ReadUInt32() -ne 0x00004550) { throw 'Missing PE signature.' }
            $machine = $reader.ReadUInt16()
            if ($machine -ne 0x8664) {
                throw ('Expected PE x86_64 machine 0x8664, found 0x{0:x4}.' -f $machine)
            }
        }
        finally {
            $reader.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

$manifest = Get-Content -LiteralPath $ManifestPath -Raw | ConvertFrom-Json
if ($manifest.schemaVersion -ne 1 -or
    $manifest.architecture -ne 'windows-x86_64') {
    throw 'Unsupported known-core manifest schema or architecture.'
}
$integrationArguments = [Collections.Generic.List[string]]::new()
foreach ($provider in $manifest.providers) {
    $providerDir = Assert-ChildPath (Join-Path $cacheRoot $provider.providerId)
    New-Item -ItemType Directory -Force -Path $providerDir | Out-Null
    $downloadName = [IO.Path]::GetFileName(([Uri] $provider.url).AbsolutePath)
    $downloadPath = Assert-ChildPath (Join-Path $providerDir $downloadName)
    $needsDownload = -not (Test-Path -LiteralPath $downloadPath)
    if (-not $needsDownload) {
        $existingDigest = (Get-FileHash -LiteralPath $downloadPath `
            -Algorithm SHA256).Hash.ToLowerInvariant()
        $needsDownload = $existingDigest -ne $provider.downloadSha256
    }
    if ($needsDownload) {
        Invoke-WebRequest -Uri $provider.url -OutFile $downloadPath
    }
    $digest = (Get-FileHash -LiteralPath $downloadPath `
        -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($digest -ne $provider.downloadSha256) {
        throw "$($provider.providerId): download SHA-256 mismatch."
    }

    $extractDir = Assert-ChildPath (Join-Path $providerDir 'extracted')
    if (Test-Path -LiteralPath $extractDir) {
        Remove-Item -LiteralPath $extractDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $extractDir | Out-Null
    if ($provider.archiveType -eq 'zip') {
        Expand-Archive -LiteralPath $downloadPath -DestinationPath $extractDir
    }
    elseif ($provider.archiveType -eq 'file') {
        Copy-Item -LiteralPath $downloadPath -Destination `
            (Join-Path $extractDir $provider.executablePath)
    }
    else {
        throw "$($provider.providerId): unsupported archive type."
    }
    $relativeExecutable = $provider.executablePath.Replace('/',
        [IO.Path]::DirectorySeparatorChar)
    $executable = Assert-ChildPath (Join-Path $extractDir $relativeExecutable)
    if (-not (Test-Path -LiteralPath $executable)) {
        throw "$($provider.providerId): expected EXE is missing: $relativeExecutable"
    }
    Assert-PeX64 $executable
    $versionOutput = Invoke-CoreProbe -Executable $executable `
        -Arguments $provider.probeArguments
    if ([string]::IsNullOrWhiteSpace($versionOutput)) {
        throw "$($provider.providerId): empty version output."
    }
    Write-Host "$($provider.providerId): $($provider.version) / $($versionOutput.Split("`n")[0])"
    $integrationArguments.Add("$($provider.providerId)=$executable")
}

$fpcCandidates = @(@(
    $(if ($env:FPC_DIR) { Join-Path $env:FPC_DIR 'fpc.exe' }),
    $(if ($env:LAZARUS_DIR) {
        Join-Path $env:LAZARUS_DIR 'fpc\3.2.2\bin\x86_64-win64\fpc.exe'
    }),
    'C:\lazarus\fpc\3.2.2\bin\x86_64-win64\fpc.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })
if (-not $fpcCandidates) {
    throw 'Pinned FPC 3.2.2 is missing.'
}
$fpc = $fpcCandidates[0]
$testBin = Join-Path $projectDir 'tests\bin'
$testLib = Join-Path $projectDir 'tests\lib'
New-Item -ItemType Directory -Force -Path $testBin,$testLib | Out-Null
$hysteriaTlsDir = Assert-ChildPath (Join-Path $cacheRoot 'hysteria-test-tls')
New-Item -ItemType Directory -Force -Path $hysteriaTlsDir | Out-Null
$hysteriaCert = Assert-ChildPath (Join-Path $hysteriaTlsDir 'localhost.crt')
$hysteriaKey = Assert-ChildPath (Join-Path $hysteriaTlsDir 'localhost.key')
$rsa = [Security.Cryptography.RSA]::Create(2048)
try {
    $request = [Security.Cryptography.X509Certificates.CertificateRequest]::new(
        'CN=localhost', $rsa,
        [Security.Cryptography.HashAlgorithmName]::SHA256,
        [Security.Cryptography.RSASignaturePadding]::Pkcs1)
    $certificate = $request.CreateSelfSigned(
        [DateTimeOffset]::UtcNow.AddDays(-1),
        [DateTimeOffset]::UtcNow.AddDays(1))
    try {
        [IO.File]::WriteAllText($hysteriaCert,
            $certificate.ExportCertificatePem())
        [IO.File]::WriteAllText($hysteriaKey,
            $rsa.ExportPkcs8PrivateKeyPem())
    }
    finally {
        $certificate.Dispose()
    }
}
finally {
    $rsa.Dispose()
}
$oldHysteriaCert = $env:ZARYA_HYSTERIA_TEST_CERT
$oldHysteriaKey = $env:ZARYA_HYSTERIA_TEST_KEY
Push-Location $projectDir
try {
    $env:ZARYA_HYSTERIA_TEST_CERT = $hysteriaCert
    $env:ZARYA_HYSTERIA_TEST_KEY = $hysteriaKey
    & $fpc '-MObjFPC' '-O2' '-Fu.' "-FU$testLib" "-FE$testBin" `
        'tests\ProviderIntegrationTest.lpr'
    if ($LASTEXITCODE -ne 0) {
        throw "ProviderIntegrationTest compilation failed with exit code $LASTEXITCODE"
    }
    & (Join-Path $testBin 'ProviderIntegrationTest.exe') @integrationArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Known-core integration matrix failed with exit code $LASTEXITCODE"
    }
}
finally {
    $env:ZARYA_HYSTERIA_TEST_CERT = $oldHysteriaCert
    $env:ZARYA_HYSTERIA_TEST_KEY = $oldHysteriaKey
    Pop-Location
}
Write-Host 'Pinned known-core download and integration matrix: PASS'
