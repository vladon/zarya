$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoDir = Split-Path -Parent (Split-Path -Parent $projectDir)
$fpcCandidates = @(@(
    (Get-Command fpc -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source -ErrorAction SilentlyContinue),
    'C:\lazarus\fpc\3.2.2\bin\x86_64-win64\fpc.exe'
) | Where-Object { $_ -and (Test-Path -LiteralPath $_) })

if (-not $fpcCandidates) {
    throw 'fpc.exe not found.'
}

$testBin = Join-Path $projectDir 'tests\bin'
$testLib = Join-Path $projectDir 'tests\lib'
New-Item -ItemType Directory -Force -Path $testBin,$testLib | Out-Null
$bridgeDll = Join-Path $repoDir 'build\Release\zarya-xray.dll'
$hasDevelopmentBridge = Test-Path -LiteralPath $bridgeDll
if ($hasDevelopmentBridge) {
    Copy-Item -LiteralPath $bridgeDll -Destination `
        (Join-Path $testBin 'zarya-xray.dll') -Force
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
    Remove-Item -LiteralPath $workerError -Force -ErrorAction SilentlyContinue
    Write-Host 'Static Zarya.exe embedded workers: PASS'
}
finally {
    Pop-Location
}
