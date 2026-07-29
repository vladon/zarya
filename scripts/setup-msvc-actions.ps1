$ErrorActionPreference = "Stop"

$before = @{}
Get-ChildItem Env: | ForEach-Object { $before[$_.Name] = $_.Value }

$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe was not found on the GitHub Actions runner."
}

$installationPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installationPath)) {
    throw "A Visual Studio installation with the MSVC x64 tools was not found."
}

$devShellModule = Join-Path $installationPath `
    "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
Import-Module $devShellModule
Enter-VsDevShell -VsInstallPath $installationPath -SkipAutomaticLocation `
    -DevCmdArguments "-arch=x64 -host_arch=x64"

if ([string]::IsNullOrWhiteSpace($env:GITHUB_ENV)) {
    throw "GITHUB_ENV is not available."
}

Get-ChildItem Env: | ForEach-Object {
    if ($before[$_.Name] -ne $_.Value) {
        if ($_.Value -match "[`r`n]") {
            throw "Cannot persist multiline MSVC environment variable $($_.Name)."
        }
        "$($_.Name)=$($_.Value)" | Out-File -FilePath $env:GITHUB_ENV `
            -Append -Encoding utf8
    }
}

Write-Host "MSVC x64 developer environment enabled from $installationPath"
