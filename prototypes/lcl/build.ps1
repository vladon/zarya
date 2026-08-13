$ErrorActionPreference = 'Stop'

$projectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
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
Push-Location $projectDir
try {
    & $lazbuild --build-mode=Default --widgetset=win32 zarya_lcl.lpi
    if ($LASTEXITCODE -ne 0) {
        throw "lazbuild failed with exit code $LASTEXITCODE"
    }
    Write-Host "Built: $projectDir\bin\zarya-lcl-prototype.exe"
}
finally {
    Pop-Location
}
