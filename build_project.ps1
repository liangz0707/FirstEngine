# FirstEngine 编译脚本
# 使用方法: .\build_project.ps1

Write-Host "Building FirstEngine..." -ForegroundColor Green

# 检查 build 目录
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# 进入 build 目录
Set-Location build

# 清理之前的构建（可选）
# if (Test-Path "CMakeCache.txt") {
#     Write-Host "Cleaning previous build..." -ForegroundColor Yellow
#     Remove-Item -Recurse -Force * -ErrorAction SilentlyContinue
# }

# 运行 CMake 配置
Write-Host "Running CMake configuration..." -ForegroundColor Yellow
$cmakeResult = & cmake .. 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "CMake configuration failed!" -ForegroundColor Red
    Write-Host $cmakeResult
    Set-Location ..
    exit 1
}

# 编译项目
Write-Host "Building project..." -ForegroundColor Yellow
$buildResult = & cmake --build . --config Release 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Write-Host $buildResult
    Set-Location ..
    exit 1
}

Write-Host "Build completed successfully!" -ForegroundColor Green
Set-Location ..
