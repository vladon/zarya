$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent (Split-Path -Parent $projectDir)
$fpcCandidates = @(@(
    $(if ($env:FPC_DIR) { Join-Path $env:FPC_DIR 'fpc.exe' }),
    $(if ($env:LAZARUS_DIR) { Join-Path $env:LAZARUS_DIR 'fpc\3.2.2\bin\x86_64-win64\fpc.exe' }),
    (Get-Command fpc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    'C:\lazarus\fpc\3.2.2\bin\x86_64-win64\fpc.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if (-not $fpcCandidates) {
    throw 'fpc.exe not found.'
}

$testBin = Join-Path $projectDir 'tests\bin'
$testLib = Join-Path $projectDir 'tests\lib'
New-Item -ItemType Directory -Force -Path $testBin,$testLib | Out-Null
$go = Get-Command go -ErrorAction SilentlyContinue |
    Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue
$gcc = if ($env:CC) { $env:CC } else {
    Join-Path $repoDir `
        'build\tools\winlibs\mingw64\bin\x86_64-w64-mingw32-gcc.exe'
}
$bridgeDir = Join-Path $repoDir 'src\runtime\embedded\xray\bridge'
$bridgeDll = Join-Path $testBin 'zarya-xray.dll'
$hasDevelopmentBridge = $go -and (Test-Path -LiteralPath $gcc)
if ($hasDevelopmentBridge) {
    $oldCgo = $env:CGO_ENABLED
    $oldCc = $env:CC
    try {
        $env:CGO_ENABLED = '1'
        $env:CC = $gcc
        Push-Location $bridgeDir
        try {
            & $go build -trimpath '-ldflags=-s -w -buildid=' `
                -buildmode=c-shared -o $bridgeDll .
            if ($LASTEXITCODE -ne 0) {
                throw "Development Xray bridge build failed with exit code $LASTEXITCODE"
            }
        }
        finally {
            Pop-Location
        }
    }
    finally {
        $env:CGO_ENABLED = $oldCgo
        $env:CC = $oldCc
    }
    Remove-Item -LiteralPath (Join-Path $testBin 'zarya-xray.h') `
        -Force -ErrorAction SilentlyContinue
}

function Invoke-ZaryaWorker {
    param([string[]] $WorkerArguments, [int] $TimeoutMilliseconds = 15000)
    $zarya = Join-Path $projectDir 'bin\Zarya.exe'
    if (-not (Test-Path -LiteralPath $zarya)) {
        throw 'Production Zarya.exe is missing. Run .\build.ps1 first.'
    }
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $zarya
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    foreach ($argument in $WorkerArguments) {
        [void] $startInfo.ArgumentList.Add($argument)
    }
    $process = [System.Diagnostics.Process]::Start($startInfo)
    try {
        if (-not $process.WaitForExit($TimeoutMilliseconds)) {
            $process.Kill()
            throw "Zarya worker timed out after $TimeoutMilliseconds ms."
        }
        return $process.ExitCode
    }
    finally {
        $process.Dispose()
    }
}

Push-Location $projectDir
try {
    & $fpcCandidates[0] '-MObjFPC' '-O2' '-Fu.' "-FU$testLib" "-FE$testBin" `
        'tests\FakeCore.lpr'
    if ($LASTEXITCODE -ne 0) {
        throw "FakeCore compilation failed with exit code $LASTEXITCODE"
    }
    $tests = [System.Collections.Generic.List[string]] @(
        'ProfileStoreTest',
        'DataMigrationTest',
        'VlessXrayTest',
        'ShareLinkTest',
        'SubscriptionTest',
        'BackupDiagnosticsTest',
        'XrayProtocolMatrixTest',
        'RuntimeFoundationTest',
        'NodeTestWorkerTest',
        'RealDelayBatchTest',
        'TcpLatencyTest',
        'CoreProviderTest',
        'ConfigAdapterTest',
        'ExternalProcessTest'
    )
    if ($hasDevelopmentBridge) {
        $tests.Add('EmbeddedValidationTest')
        $tests.Add('EmbeddedRuntimeTest')
    } else {
        Write-Warning 'Development zarya-xray.dll not found; optional DLL ABI tests skipped.'
    }
    foreach ($testName in $tests) {
        & $fpcCandidates[0] '-MObjFPC' '-O2' '-Fu.' "-FU$testLib" "-FE$testBin" `
            "tests\$testName.lpr"
        if ($LASTEXITCODE -ne 0) {
            throw "$testName compilation failed with exit code $LASTEXITCODE"
        }
        & (Join-Path $testBin "$testName.exe")
        if ($LASTEXITCODE -ne 0) {
            throw "$testName failed with exit code $LASTEXITCODE"
        }
    }

    $xrayFixture = Join-Path $repoDir 'build\Release\cores\xray\xray.exe'
    $singBoxFixture = Join-Path $repoDir 'build\Release\cores\sing-box\sing-box.exe'
    if ((Test-Path -LiteralPath $xrayFixture) -and
        (Test-Path -LiteralPath $singBoxFixture)) {
        $integrationTest = 'ProviderIntegrationTest'
        & $fpcCandidates[0] '-MObjFPC' '-O2' '-Fu.' "-FU$testLib" `
            "-FE$testBin" "tests\$integrationTest.lpr"
        if ($LASTEXITCODE -ne 0) {
            throw "$integrationTest compilation failed with exit code $LASTEXITCODE"
        }
        & (Join-Path $testBin "$integrationTest.exe") `
            $xrayFixture $singBoxFixture
        if ($LASTEXITCODE -ne 0) {
            throw "$integrationTest failed with exit code $LASTEXITCODE"
        }
    } else {
        Write-Warning 'Real external Xray/sing-box fixtures not found; integration smoke skipped.'
    }

    $workerDir = Join-Path $testBin 'worker-assets'
    $workerError = Join-Path $testBin 'worker-error.txt'
    New-Item -ItemType Directory -Force -Path $workerDir | Out-Null
    Remove-Item -LiteralPath $workerError -Force -ErrorAction SilentlyContinue
    $exitCode = Invoke-ZaryaWorker -WorkerArguments @('--embedded-abi-test')
    if ($exitCode -ne 0) {
        throw "Static embedded ABI worker failed with exit code $exitCode"
    }
    $exitCode = Invoke-ZaryaWorker -WorkerArguments @(
        '--embedded-validate',
        (Join-Path $projectDir 'tests\data\minimal-xray.json'),
        $workerDir, $workerError)
    if ($exitCode -ne 0) {
        throw "Static embedded validation worker failed: $(Get-Content $workerError -Raw)"
    }
    $exitCode = Invoke-ZaryaWorker -WorkerArguments @(
        '--embedded-validate',
        (Join-Path $projectDir 'tests\data\invalid-xray.json'),
        $workerDir, $workerError)
    if ($exitCode -eq 0) {
        throw 'Static embedded validation accepted invalid JSON.'
    }
    $listener = [System.Net.Sockets.TcpListener]::new(
        [System.Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $runtimePort = ([System.Net.IPEndPoint] $listener.LocalEndpoint).Port
    $listener.Stop()
    $exitCode = Invoke-ZaryaWorker -WorkerArguments @(
        '--embedded-runtime-smoke', $workerDir,
        [string] $runtimePort, $workerError)
    if ($exitCode -ne 0) {
        throw "Static embedded runtime worker failed: $(Get-Content $workerError -Raw)"
    }

    # End-to-end Real delay smoke: worker -> embedded mixed endpoint -> local
    # HTTP target. The target stays local, so the test is deterministic and
    # does not modify WinINet or depend on internet access.
    $httpListener = [System.Net.Sockets.TcpListener]::new(
        [System.Net.IPAddress]::Loopback, 0)
    $httpListener.Start()
    $httpPort = ([System.Net.IPEndPoint] $httpListener.LocalEndpoint).Port
    $runtimeListener = [System.Net.Sockets.TcpListener]::new(
        [System.Net.IPAddress]::Loopback, 0)
    $runtimeListener.Start()
    $realDelayPort = ([System.Net.IPEndPoint] $runtimeListener.LocalEndpoint).Port
    $runtimeListener.Stop()
    $realDelayConfig = [ordered]@{
        log = @{ loglevel = 'none' }
        inbounds = @(@{
            listen = '127.0.0.1'; port = $realDelayPort
            protocol = 'mixed'; tag = 'mixed-in'; settings = @{ udp = $false }
        })
        outbounds = @(@{ protocol = 'freedom'; tag = 'direct' })
    } | ConvertTo-Json -Depth 8 -Compress
    $realDelayRequest = [ordered]@{
        schemaVersion = 1
        provider = [ordered]@{
            providerId = 'embedded.xray'; distribution = 'embedded'
            executablePath = ''; workingDirectory = ''; assetDirectory = $workerDir
            configExtension = '.json'; confirmedSha256 = ''
            validateArguments = @(); runArguments = @()
        }
        config = $realDelayConfig
        dataDirectory = $workerDir
        assetDirectory = $workerDir
        readinessHost = '127.0.0.1'
        readinessPort = $realDelayPort
        proxyKind = 'mixed'
        testUrl = "http://127.0.0.1:$httpPort/generate_204"
        timeoutMs = 10000
    } | ConvertTo-Json -Depth 8 -Compress
    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = Join-Path $projectDir 'bin\Zarya.exe'
    $startInfo.ArgumentList.Add('--core-test-worker')
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $realDelayProcess = [System.Diagnostics.Process]::Start($startInfo)
    try {
        $realDelayProcess.StandardInput.WriteLine($realDelayRequest)
        $realDelayProcess.StandardInput.Flush()
        $acceptTask = $httpListener.AcceptTcpClientAsync()
        $acceptDeadline = [DateTime]::UtcNow.AddSeconds(12)
        while (-not $acceptTask.IsCompleted -and
            -not $realDelayProcess.HasExited -and
            [DateTime]::UtcNow -lt $acceptDeadline) {
            Start-Sleep -Milliseconds 20
        }
        if (-not $acceptTask.IsCompleted) {
            if (-not $realDelayProcess.HasExited) {
                $realDelayProcess.Kill()
                $realDelayProcess.WaitForExit()
            }
            $earlyOutput = $realDelayProcess.StandardOutput.ReadToEnd()
            $earlyError = $realDelayProcess.StandardError.ReadToEnd()
            throw "Real delay target did not receive a proxied request: $earlyOutput $earlyError"
        }
        $client = $acceptTask.Result
        try {
            $stream = $client.GetStream()
            $buffer = [byte[]]::new(8192)
            do {
                $read = $stream.Read($buffer, 0, $buffer.Length)
                $requestText = [Text.Encoding]::ASCII.GetString($buffer, 0, $read)
            } while ($read -gt 0 -and $requestText -notmatch "\r\n\r\n")
            $responseBytes = [Text.Encoding]::ASCII.GetBytes(
                "HTTP/1.1 204 No Content`r`nConnection: close`r`nContent-Length: 0`r`n`r`n")
            $stream.Write($responseBytes, 0, $responseBytes.Length)
            $stream.Flush()
        }
        finally {
            $client.Dispose()
        }
        if (-not $realDelayProcess.WaitForExit(5000)) {
            throw 'Real delay worker did not exit after the HTTP response.'
        }
        $realDelayOutput = $realDelayProcess.StandardOutput.ReadToEnd()
        $realDelayError = $realDelayProcess.StandardError.ReadToEnd()
        $realDelayResult = $realDelayOutput.Trim().Split("`n")[-1] |
            ConvertFrom-Json
        if ($realDelayProcess.ExitCode -ne 0 -or -not $realDelayResult.success -or
            $realDelayResult.delayMs -lt 0) {
            throw "Real delay worker failed: $realDelayOutput $realDelayError"
        }
    }
    finally {
        if (-not $realDelayProcess.HasExited) { $realDelayProcess.Kill() }
        $realDelayProcess.Dispose()
        $httpListener.Stop()
    }
    Remove-Item -LiteralPath $workerError -Force -ErrorAction SilentlyContinue
    Write-Host 'Static Zarya.exe embedded workers and Real delay: PASS'
}
finally {
    Pop-Location
}
