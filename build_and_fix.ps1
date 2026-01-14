# FirstEngine 编译和修复脚本
# 这个脚本会尝试编译项目，并显示详细的错误信息

param(
    [switch]$Clean,
    [string]$BuildType = "Release",
    [string]$Generator = "Visual Studio 17 2022"
)

$ErrorActionPreference = "Continue"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $scriptDir

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FirstEngine Build Script" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# 检查 CMakeLists.txt
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "ERROR: CMakeLists.txt not found!" -ForegroundColor Red
    Write-Host "Current directory: $(Get-Location)" -ForegroundColor Yellow
    exit 1
}

# 检查必要的目录
Write-Host "Checking project structure..." -ForegroundColor Yellow
$requiredDirs = @("src", "include", "third_party")
foreach ($dir in $requiredDirs) {
    if (-not (Test-Path $dir)) {
        Write-Host "WARNING: Directory '$dir' not found!" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ $dir" -ForegroundColor Green
    }
}

# 创建 build 目录
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# 清理（如果指定）
if ($Clean -and (Test-Path "build\CMakeCache.txt")) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force build\* -ErrorAction SilentlyContinue
}

Set-Location build

Write-Host ""
Write-Host "Running CMake configuration..." -ForegroundColor Yellow
Write-Host "Generator: $Generator" -ForegroundColor Gray
Write-Host "Build Type: $BuildType" -ForegroundColor Gray
Write-Host ""

# 运行 CMake
$cmakeArgs = @("..")
if ($Generator -ne "") {
    $cmakeArgs += @("-G", $Generator)
    if ($Generator -like "*Visual Studio*") {
        $cmakeArgs += @("-A", "x64")
    }
}

$cmakeOutput = & cmake @cmakeArgs 2>&1
$cmakeExitCode = $LASTEXITCODE

if ($cmakeExitCode -ne 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "CMake Configuration FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host $cmakeOutput
    Write-Host ""
    Write-Host "Common issues:" -ForegroundColor Yellow
    Write-Host "1. Missing Vulkan SDK - install from https://vulkan.lunarg.com/" -ForegroundColor White
    Write-Host "2. Missing Git submodules - run: git submodule update --init --recursive" -ForegroundColor White
    Write-Host "3. Wrong CMake version - requires CMake >= 3.20" -ForegroundColor White
    Write-Host "4. Missing Visual Studio or compiler" -ForegroundColor White
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "CMake configuration successful!" -ForegroundColor Green
Write-Host ""

# 编译项目
Write-Host "Building project..." -ForegroundColor Yellow
$buildOutput = & cmake --build . --config $BuildType 2>&1
$buildExitCode = $LASTEXITCODE

if ($buildExitCode -ne 0) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "Build FAILED!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    # 只显示最后 50 行错误信息
    $buildOutput | Select-Object -Last 50
    Write-Host ""
    Write-Host "Check the errors above and fix the issues." -ForegroundColor Yellow
    Set-Location ..
    exit 1
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Green
Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""
Write-Host "Executable location:" -ForegroundColor Cyan
if (Test-Path "bin\$BuildType\FirstEngine.exe") {
    Write-Host "  $(Resolve-Path "bin\$BuildType\FirstEngine.exe")" -ForegroundColor White
} elseif (Test-Path "bin\FirstEngine") {
    Write-Host "  $(Resolve-Path "bin\FirstEngine")" -ForegroundColor White
} else {
    Write-Host "  (not found - check build output)" -ForegroundColor Yellow
}

Set-Location ..
Write-Host ""
