# Build a minimal static Qt 6.8 (qtbase + qtsvg) for MSVC / VS 2026.
# Output: C:\Qt\Static\<version>\msvc2022_64
#
#   .\scripts\build-qt-static-msvc2026.ps1           # qtbase + qtsvg (full)
#   .\scripts\build-qt-static-msvc2026.ps1 -SvgOnly  # qtsvg only against existing prefix
param(
    [switch]$SvgOnly
)

$ErrorActionPreference = "Stop"

$QtVersion = if ($env:QT_VERSION) { $env:QT_VERSION } else { "6.8.3" }
$Prefix = if ($env:QT_STATIC_DIR) { $env:QT_STATIC_DIR } else { "C:\Qt\Static\$QtVersion\msvc2022_64" }
$SrcRoot = if ($env:QT_STATIC_SRC) { $env:QT_STATIC_SRC } else { "C:\Qt\Static\src" }
$QtBaseSrc = Join-Path $SrcRoot "$QtVersion\Src\qtbase"
$QtSvgSrc = Join-Path $SrcRoot "$QtVersion\Src\qtsvg"
$BuildDir = if ($env:QT_STATIC_BUILD_DIR) { $env:QT_STATIC_BUILD_DIR } else { "C:\Qt\Static\build\qtbase-msvc2022_64" }
$SvgBuildDir = if ($env:QT_STATIC_SVG_BUILD_DIR) { $env:QT_STATIC_SVG_BUILD_DIR } else { "C:\Qt\Static\build\qtsvg-msvc2022_64" }
$Ninja = if ($env:QT_NINJA) { $env:QT_NINJA } else { "C:\Qt\Tools\Ninja\ninja.exe" }

$VcVars = if ($env:QT_VCVARS) {
    $env:QT_VCVARS
} else {
    "${env:ProgramFiles}\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"
}
if (-not (Test-Path $VcVars)) {
    Write-Error "VS vcvars64.bat not found at $VcVars (set QT_VCVARS to override)"
}

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

function Ensure-Ninja {
    if (-not (Test-Path $Ninja)) {
        Write-Host "Installing Ninja..."
        python -m aqt install-tool windows desktop tools_ninja -O C:\Qt
    }
    if (-not (Test-Path $Ninja)) {
        Write-Error "Ninja not found at $Ninja"
    }
}

function Ensure-QtBaseSource {
    if (-not (Test-Path $QtBaseSrc)) {
        Write-Host "Downloading Qt $QtVersion qtbase source..."
        python -m aqt install-src windows $QtVersion -O $SrcRoot --archives qtbase
    }
    if (-not (Test-Path $QtBaseSrc)) {
        Write-Error "qtbase source missing at $QtBaseSrc"
    }
}

function Ensure-QtSvgSource {
    if (-not (Test-Path $QtSvgSrc)) {
        Write-Host "Downloading Qt $QtVersion qtsvg source..."
        python -m aqt install-src windows $QtVersion -O $SrcRoot --archives qtsvg
    }
    if (-not (Test-Path $QtSvgSrc)) {
        Write-Error "qtsvg source missing at $QtSvgSrc"
    }
}

function Build-StaticQtBase {
    Ensure-QtBaseSource
    Ensure-Ninja

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
}

function Build-StaticQtSvg {
    if (-not (Test-Path "$Prefix\lib\cmake\Qt6\Qt6Config.cmake")) {
        Write-Error "Static Qt prefix missing at $Prefix. Run without -SvgOnly first."
    }

    Ensure-QtSvgSource
    Ensure-Ninja

    if (Test-Path $SvgBuildDir) {
        Remove-Item -Recurse -Force $SvgBuildDir
    }
    New-Item -ItemType Directory -Force -Path $SvgBuildDir | Out-Null

    Write-Host "Configuring static qtsvg (Ninja + MSVC) -> $Prefix"
    Invoke-VsCMake @(
        "-S", $QtSvgSrc,
        "-B", $SvgBuildDir,
        "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=$Prefix",
        "-DCMAKE_PREFIX_PATH=$Prefix",
        "-DCMAKE_C_COMPILER=cl",
        "-DCMAKE_CXX_COMPILER=cl",
        "-DCMAKE_MAKE_PROGRAM=$Ninja",
        "-DBUILD_SHARED_LIBS=OFF",
        "-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded",
        "-DQT_BUILD_EXAMPLES=OFF",
        "-DQT_BUILD_TESTS=OFF"
    )

    Write-Host "Building static qtsvg..."
    Invoke-VsCMake @("--build", $SvgBuildDir, "--parallel")

    Write-Host "Installing static qtsvg..."
    Invoke-VsCMake @("--install", $SvgBuildDir)

    if (-not (Test-Path "$Prefix\lib\cmake\Qt6Svg\Qt6SvgConfig.cmake")) {
        Write-Error "Install failed: Qt6SvgConfig.cmake not found under $Prefix"
    }
}

if (-not $SvgOnly) {
    Build-StaticQtBase
}
Build-StaticQtSvg

Write-Host "Static Qt (with Svg) installed to $Prefix"
