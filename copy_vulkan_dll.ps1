# 复制 vulkan-1.dll 到项目

Write-Host "查找并复制 vulkan-1.dll..." -ForegroundColor Cyan
Write-Host ""

$targetDir = "external\vulkan"
$found = $false

# 1. 检查系统目录
$systemDll = "C:\Windows\System32\vulkan-1.dll"
if (Test-Path $systemDll) {
    Write-Host "[找到] 系统 DLL: $systemDll" -ForegroundColor Green
    
    # 确保目标目录存在
    New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
    
    # 复制 DLL
    Copy-Item $systemDll "$targetDir\vulkan-1.dll" -Force
    Write-Host "[已复制] 到 $targetDir\vulkan-1.dll" -ForegroundColor Green
    $found = $true
}

# 2. 检查 VULKAN_SDK
if (-not $found -and $env:VULKAN_SDK) {
    $sdkDll = "$env:VULKAN_SDK\Bin\vulkan-1.dll"
    if (Test-Path $sdkDll) {
        Write-Host "[找到] SDK DLL: $sdkDll" -ForegroundColor Green
        New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
        Copy-Item $sdkDll "$targetDir\vulkan-1.dll" -Force
        Write-Host "[已复制] 到 $targetDir\vulkan-1.dll" -ForegroundColor Green
        $found = $true
    }
}

# 3. 检查常见 SDK 路径
if (-not $found) {
    $sdkPaths = @(
        "D:\VulkanSDK\1.3.275.0\Bin\vulkan-1.dll",
        "C:\VulkanSDK\1.3.275.0\Bin\vulkan-1.dll",
        "D:\VulkanSDK\1.3.250.0\Bin\vulkan-1.dll",
        "C:\VulkanSDK\1.3.250.0\Bin\vulkan-1.dll"
    )
    
    foreach ($path in $sdkPaths) {
        if (Test-Path $path) {
            Write-Host "[找到] SDK DLL: $path" -ForegroundColor Green
            New-Item -ItemType Directory -Force -Path $targetDir | Out-Null
            Copy-Item $path "$targetDir\vulkan-1.dll" -Force
            Write-Host "[已复制] 到 $targetDir\vulkan-1.dll" -ForegroundColor Green
            $found = $true
            break
        }
    }
}

if ($found) {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Green
    Write-Host "成功！vulkan-1.dll 已复制到项目" -ForegroundColor Green
    Write-Host "========================================" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步:" -ForegroundColor Yellow
    Write-Host "1. 重新生成 CMake 项目" -ForegroundColor White
    Write-Host "2. 重新编译项目" -ForegroundColor White
} else {
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Red
    Write-Host "未找到 vulkan-1.dll!" -ForegroundColor Red
    Write-Host "========================================" -ForegroundColor Red
    Write-Host ""
    Write-Host "请:" -ForegroundColor Yellow
    Write-Host "1. 安装 Vulkan Runtime: https://vulkan.lunarg.com/sdk/home#runtime" -ForegroundColor White
    Write-Host "2. 或更新显卡驱动（通常包含 Vulkan Runtime）" -ForegroundColor White
}
