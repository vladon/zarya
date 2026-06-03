# Build a minimal static Qt 6.8 qtbase for MSVC / VS 2026.
# Output: C:\Qt\Static\<version>\msvc2022_64
$ErrorActionPreference = "Stop"

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$Prefix = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
$SrcRoot = if ($env:QT_STATIC_SRC) { $env:QT_STATIC_SRC } else { "C:\Qt\Static\src" }
$QtBaseSrc = Join-Path $SrcRoot "$QtVersion\Src\qtbase"
$BuildDir = if ($env:QT_STATIC_BUILD_DIR) { $env:QT_STATIC_BUILD_DIR } else { "C:\Qt\Static\build\qtbase-msvc2022_64" }
$Ninja = if ($env:QT_NINJA) { $env:QT_NINJA } else { "C:\Qt\Tools\Ninja\ninja.exe" }

$VcVars = "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $VcVars)) {
    Write-Error "VS 2026 vcvars64.bat not found at $VcVars"
}

if (-not (Test-Path $QtBaseSrc)) {
    Write-Host "Downloading Qt $QtVersion qtbase source..."
    python -m aqt install-src windows $QtVersion -O $SrcRoot --archives qtbase
}

if (-not (Test-Path $QtBaseSrc)) {
    Write-Error "qtbase source missing at $QtBaseSrc"
}

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

function Invoke-VsCMake([string[]]$CMakeArgs) {
    $argLine = ($CMakeArgs | ForEach-Object { "`"$($_ -replace '"','\"')`"" }) -join ' '
    $batch = "@echo off`r`ncall `"$VcVars`" >nul`r`ncmake $argLine`r`nexit /b %ERRORLEVEL%"
    $batchFile = Join-Path $env:TEMP "zarya-qt-static-cmd.bat"
    Set-Content -Path $batchFile -Value $batch -Encoding ASCII
    & cmd /c $batchFile
    if ($LASTEXITCODE -ne 0) {
        throw "cmake failed (exit $LASTEXITCODE): cmake $argLine"
    }
}

if (-not (Test-Path $Ninja)) {
    Write-Host "Installing Ninja..."
    python -m aqt install-tool windows desktop tools_ninja -O C:\Qt
}

if (Test-Path $BuildDir) {
    Remove-Item -Recurse -Force $BuildDir
}
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

Write-Host "Configuring static qtbase (Ninja + MSVC) -> $Prefix"
Invoke-VsCMake @(
    "-S", $QtBaseSrc,
    "-B", $BuildDir,
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_INSTALL_PREFIX=$Prefix",
    "-DCMAKE_C_COMPILER=cl",
    "-DCMAKE_CXX_COMPILER=cl",
    "-DCMAKE_MAKE_PROGRAM=$Ninja",
    "-DBUILD_SHARED_LIBS=OFF",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
    "-DQT_BUILD_EXAMPLES=OFF",
    "-DQT_BUILD_TESTS=OFF"
)

Write-Host "Building static qtbase..."
Invoke-VsCMake @("--build", $BuildDir, "--parallel")

Write-Host "Installing static qtbase..."
Invoke-VsCMake @("--install", $BuildDir)

if (-not (Test-Path "$Prefix\lib\cmake\Qt6\Qt6Config.cmake")) {
    Write-Error "Install failed: Qt6Config.cmake not found under $Prefix"
}

Write-Host "Static Qt installed to $Prefix"
