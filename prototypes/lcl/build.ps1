$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent (Split-Path -Parent $projectDir)
$candidates = @(@(
    (Get-Command lazbuild -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    'C:\lazarus\lazbuild.exe',
    'C:\Program Files\Lazarus\lazbuild.exe',
    'C:\Program Files (x86)\Lazarus\lazbuild.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if (-not $candidates) {
    throw 'lazbuild.exe not found. Install Lazarus, for example: winget install --id Lazarus.Lazarus'
}

$lazbuild = $candidates[0]
$fpcCandidates = @(@(
    (Get-Command fpc -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    'C:\lazarus\fpc\3.2.2\bin\x86_64-win64\fpc.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })
if (-not $fpcCandidates) {
    throw '64-bit fpc.exe not found.'
}
$fpc = $fpcCandidates[0]
$gccCandidates = @(@(
    $env:CC,
    (Join-Path $repoDir 'build\tools\winlibs\mingw64\bin\x86_64-w64-mingw32-gcc.exe'),
    (Get-Command x86_64-w64-mingw32-gcc -ErrorAction SilentlyContinue |
        Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue)
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })
if (-not $gccCandidates) {
    throw 'MinGW-w64 x86_64 compiler not found. Set CC to x86_64-w64-mingw32-gcc.exe.'
}

$go = Get-Command go -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue
if (-not $go) {
    throw 'go.exe not found. Go is required to build the embedded Xray archive.'
}

$gcc = $gccCandidates[0]
$fpcBin = Split-Path -Parent $fpc
$fpcLinker = Join-Path $fpcBin 'ld.exe'
$mingwRoot = Split-Path -Parent (Split-Path -Parent $gcc)
$targetLib = Join-Path $mingwRoot 'x86_64-w64-mingw32\lib'
$gccLib = Get-ChildItem (Join-Path $mingwRoot 'lib\gcc\x86_64-w64-mingw32') `
    -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not (Test-Path -LiteralPath $fpcLinker) -or
    -not (Test-Path -LiteralPath $targetLib) -or -not $gccLib) {
    throw "MinGW-w64 import libraries not found below $mingwRoot"
}

$generatedDir = Join-Path $projectDir 'generated'
$archive = Join-Path $generatedDir 'libzarya_xray_static.a'
$linkerDir = Join-Path $generatedDir 'fpc-linker'
$largeAddressLinker = Join-Path $linkerDir 'ld.exe'
$bridgeDir = Join-Path $repoDir 'src\runtime\embedded\xray\bridge'
$binDir = Join-Path $projectDir 'bin'
$output = Join-Path $binDir 'Zarya.exe'
$checksum = $output + '.sha256'
New-Item -ItemType Directory -Force -Path $generatedDir, $linkerDir, $binDir | Out-Null

function Remove-BinArtifact {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path
    )
    if (-not (Test-Path -LiteralPath $Path)) {
        return
    }
    $resolvedBin = [System.IO.Path]::GetFullPath($binDir).TrimEnd('\')
    $resolvedPath = [System.IO.Path]::GetFullPath($Path)
    if ([System.IO.Path]::GetDirectoryName($resolvedPath) -ne $resolvedBin) {
        throw "Refusing to remove an artifact outside $resolvedBin`: $resolvedPath"
    }
    Remove-Item -LiteralPath $resolvedPath -Force
}

function Remove-TemporaryBinArtifacts {
    Get-ChildItem -LiteralPath $binDir -Filter 'link*.res' -File |
        ForEach-Object { Remove-BinArtifact -Path $_.FullName }
    @(
        'ppas.bat',
        'zarya-lcl-prototype.exe',
        'Zarya-nogc.exe',
        'zarya-xray.dll'
    ) | ForEach-Object { Remove-BinArtifact -Path (Join-Path $binDir $_) }
}

# A failed rebuild must not leave an obsolete checksum looking current.
Remove-BinArtifact -Path $checksum
Remove-TemporaryBinArtifacts

function Invoke-EmbeddedSelfTest {
    param(
        [Parameter(Mandatory = $true)]
        [string[]] $SelfTestArguments,
        [int] $TimeoutMilliseconds = 15000
    )
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $output
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in $SelfTestArguments) {
        [void] $startInfo.ArgumentList.Add($argument)
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    try {
        if (-not $process.WaitForExit($TimeoutMilliseconds)) {
            $process.Kill()
            throw "Embedded self-test timed out after $TimeoutMilliseconds ms."
        }
        $standardOutput = $process.StandardOutput.ReadToEnd()
        $standardError = $process.StandardError.ReadToEnd()
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = ($standardOutput + $standardError).Trim()
        }
    }
    finally {
        $process.Dispose()
    }
}

# FPC's linker understands its COFF objects. The Go archive requires more than
# the default 2 GB address space, so make a local LARGEADDRESSAWARE copy.
Copy-Item -LiteralPath $fpcLinker -Destination $largeAddressLinker -Force
$linkerBytes = [System.IO.File]::ReadAllBytes($largeAddressLinker)
$peOffset = [System.BitConverter]::ToInt32($linkerBytes, 0x3c)
$characteristicsOffset = $peOffset + 22
$characteristics = [System.BitConverter]::ToUInt16($linkerBytes, $characteristicsOffset)
$characteristics = $characteristics -bor 0x20
$characteristicBytes = [System.BitConverter]::GetBytes([uint16]$characteristics)
$linkerBytes[$characteristicsOffset] = $characteristicBytes[0]
$linkerBytes[$characteristicsOffset + 1] = $characteristicBytes[1]
[System.IO.File]::WriteAllBytes($largeAddressLinker, $linkerBytes)

$oldCgo = $env:CGO_ENABLED
$oldCc = $env:CC
Push-Location $projectDir
try {
    $env:CGO_ENABLED = '1'
    $env:CC = $gcc
    Push-Location $bridgeDir
    try {
        & $go build -trimpath '-ldflags=-s -w -buildid=' -buildmode=c-archive `
            -o $archive .
        if ($LASTEXITCODE -ne 0) {
            throw "Go c-archive build failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }

    $buildStarted = Get-Date
    $linkOptions = @(
        '-dZARYA_STATIC_XRAY',
        '-Xe',
        '-sh',
        "-k-L$targetLib",
        "-k-L$gccLib",
        '-k--whole-archive',
        "-k$archive",
        '-k--no-whole-archive',
        '-k-lmsvcrt', '-k-lkernel32', '-k-lws2_32', '-k-lwinmm',
        '-k-lntdll', '-k-lbcrypt', '-k-liphlpapi', '-k-lsecur32',
        '-k-ladvapi32', '-k-luserenv', '-k-lshell32', '-k-lole32',
        '-k-loleaut32', '-k-luuid'
    )
    $compilerOptions = '--opt=' + ($linkOptions -join ' ')
    # Rebuild all project units: the static ABI define changes ZaryaEmbeddedXray
    # and Lazarus does not include command-line defines in its timestamp cache.
    & $lazbuild --build-all --build-mode=Default --widgetset=win32 `
        $compilerOptions zarya_lcl.lpi
    if ($LASTEXITCODE -ne 0) {
        throw "Pascal compilation failed with exit code $LASTEXITCODE"
    }

    $linkResponse = Get-ChildItem -LiteralPath $binDir -Filter 'link*.res' -File |
        Where-Object { $_.LastWriteTime -ge $buildStarted.AddSeconds(-2) } |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    if (-not $linkResponse) {
        throw 'FPC did not generate the final linker response file.'
    }
    if (Test-Path -LiteralPath $output) {
        Remove-Item -LiteralPath $output -Force
    }
    $linkArguments = @(
        '-b', 'pei-x86-64', "-L$targetLib", "-L$gccLib",
        '--whole-archive', $archive, '--no-whole-archive',
        '-lmsvcrt', '-lkernel32', '-lws2_32', '-lwinmm',
        '-lntdll', '-lbcrypt', '-liphlpapi', '-lsecur32',
        '-ladvapi32', '-luserenv', '-lshell32', '-lole32',
        '-loleaut32', '-luuid', '--gc-sections', '--no-gc-sections',
        '-s', '--subsystem', 'windows', '--entry=_WinMainCRTStartup',
        '-o', $output, $linkResponse.FullName
    )
    & $largeAddressLinker @linkArguments
    if ($LASTEXITCODE -ne 0) {
        throw "Static executable link failed with exit code $LASTEXITCODE"
    }

    $selfTest = Invoke-EmbeddedSelfTest `
        -SelfTestArguments @('--embedded-abi-test')
    if ($selfTest.ExitCode -ne 0) {
        throw "Embedded Xray ABI self-test failed: $($selfTest.Output)"
    }
    $validConfig = Join-Path $projectDir 'tests\data\minimal-xray.json'
    $invalidConfig = Join-Path $projectDir 'tests\data\invalid-xray.json'
    $validationAssets = Join-Path $generatedDir 'validation-assets'
    $validationError = Join-Path $generatedDir 'validation-error.txt'
    New-Item -ItemType Directory -Force -Path $validationAssets | Out-Null
    Remove-Item -LiteralPath $validationError -Force -ErrorAction SilentlyContinue
    $selfTest = Invoke-EmbeddedSelfTest -SelfTestArguments @(
        '--embedded-validate', $validConfig, $validationAssets,
        $validationError)
    if ($selfTest.ExitCode -ne 0) {
        throw "Embedded Xray validation self-test failed: $($selfTest.Output)"
    }
    $selfTest = Invoke-EmbeddedSelfTest -SelfTestArguments @(
        '--embedded-validate', $invalidConfig, $validationAssets,
        $validationError)
    if ($selfTest.ExitCode -eq 0) {
        throw 'Embedded Xray validation accepted an invalid configuration.'
    }
    $listener = [System.Net.Sockets.TcpListener]::new(
        [System.Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $runtimePort = ([System.Net.IPEndPoint] $listener.LocalEndpoint).Port
    $listener.Stop()
    $selfTest = Invoke-EmbeddedSelfTest -SelfTestArguments @(
        '--embedded-runtime-smoke', $validationAssets,
        [string] $runtimePort, $validationError)
    if ($selfTest.ExitCode -ne 0) {
        $workerError = if (Test-Path -LiteralPath $validationError) {
            Get-Content -LiteralPath $validationError -Raw
        } else {
            $selfTest.Output
        }
        throw "Embedded Xray runtime self-test failed: $workerError"
    }
    Remove-Item -LiteralPath $validationError -Force -ErrorAction SilentlyContinue
    $digest = (Get-FileHash -LiteralPath $output -Algorithm SHA256).Hash.ToLowerInvariant()
    Set-Content -LiteralPath $checksum -Encoding ascii -NoNewline `
        -Value "$digest *Zarya.exe`n"
    Remove-TemporaryBinArtifacts
    $unexpectedArtifacts = @(Get-ChildItem -LiteralPath $binDir -File |
        Where-Object { $_.Name -notin @('Zarya.exe', 'Zarya.exe.sha256') })
    if ($unexpectedArtifacts.Count -ne 0) {
        throw "Unexpected release artifacts in $binDir`: $($unexpectedArtifacts.Name -join ', ')"
    }
    Write-Host "Built: $output (embedded Xray; one runtime EXE)"
    Write-Host "SHA-256: $digest"
}
finally {
    $env:CGO_ENABLED = $oldCgo
    $env:CC = $oldCc
    Pop-Location
}
