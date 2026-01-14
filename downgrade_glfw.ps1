# GLFW 降级脚本

param(
    [string]$Version = "3.3.8"
)

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "GLFW 降级工具" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

if (-not (Test-Path "third_party\glfw")) {
    Write-Host "错误: 找不到 third_party\glfw 目录" -ForegroundColor Red
    Write-Host "请确保在项目根目录运行此脚本" -ForegroundColor Yellow
    exit 1
}

Push-Location "third_party\glfw"

try {
    # 检查是否是 Git 仓库
    if (-not (Test-Path ".git")) {
        Write-Host "警告: third_party\glfw 不是 Git 仓库" -ForegroundColor Yellow
        Write-Host "无法使用 Git 切换版本" -ForegroundColor Yellow
        Write-Host ""
        Write-Host "请手动:" -ForegroundColor Yellow
        Write-Host "1. 从 https://github.com/glfw/glfw/releases 下载 GLFW $Version" -ForegroundColor White
        Write-Host "2. 解压并替换 third_party\glfw 目录" -ForegroundColor White
        exit 1
    }
    
    # 检查当前版本
    $currentVersion = git describe --tags 2>$null
    if ($LASTEXITCODE -eq 0) {
        Write-Host "当前版本: $currentVersion" -ForegroundColor Yellow
    } else {
        Write-Host "当前版本: 未知（可能是开发版本）" -ForegroundColor Yellow
    }
    
    Write-Host ""
    Write-Host "检查版本 $Version 是否存在..." -ForegroundColor Yellow
    
    # 获取所有标签
    git fetch --tags 2>$null
    
    # 检查版本是否存在
    $tagExists = git tag -l $Version
    if (-not $tagExists) {
        Write-Host "错误: 版本 $Version 不存在" -ForegroundColor Red
        Write-Host ""
        Write-Host "可用的 3.3.x 和 3.4.x 版本:" -ForegroundColor Yellow
        git tag | Select-String "^3\.[34]\." | Sort-Object -Descending | Select-Object -First 10
        exit 1
    }
    
    Write-Host "找到版本 $Version" -ForegroundColor Green
    Write-Host ""
    Write-Host "切换到版本 $Version..." -ForegroundColor Yellow
    
    # 切换到指定版本
    git checkout $Version
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Green
        Write-Host "成功切换到 GLFW $Version" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Green
        Write-Host ""
        
        # 验证版本
        $newVersion = git describe --tags
        Write-Host "当前版本: $newVersion" -ForegroundColor Green
        Write-Host ""
        
        Write-Host "下一步操作:" -ForegroundColor Yellow
        Write-Host "1. 删除 build 目录（如果存在）" -ForegroundColor White
        Write-Host "   Remove-Item -Recurse -Force build" -ForegroundColor Gray
        Write-Host ""
        Write-Host "2. 重新生成 CMake 项目" -ForegroundColor White
        Write-Host "   mkdir build" -ForegroundColor Gray
        Write-Host "   cd build" -ForegroundColor Gray
        Write-Host "   cmake .. -G `"Visual Studio 17 2022`" -A x64" -ForegroundColor Gray
        Write-Host ""
        Write-Host "3. 重新编译项目" -ForegroundColor White
        Write-Host "   cmake --build . --config Release" -ForegroundColor Gray
    } else {
        Write-Host ""
        Write-Host "错误: 切换版本失败" -ForegroundColor Red
        Write-Host "可能的原因:" -ForegroundColor Yellow
        Write-Host "  - 有未提交的更改" -ForegroundColor White
        Write-Host "  - Git 仓库损坏" -ForegroundColor White
        exit 1
    }
} finally {
    Pop-Location
}
