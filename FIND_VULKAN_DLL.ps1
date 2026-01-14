# 查找 vulkan-1.dll 的脚本

Write-Host "正在查找 vulkan-1.dll..." -ForegroundColor Cyan
Write-Host ""

$foundDlls = @()

# 1. 检查环境变量 VULKAN_SDK
if ($env:VULKAN_SDK) {
    Write-Host "检查 VULKAN_SDK 环境变量: $env:VULKAN_SDK" -ForegroundColor Yellow
    
    $paths = @(
        "$env:VULKAN_SDK\Bin\vulkan-1.dll",
        "$env:VULKAN_SDK\vulkan-1.dll",
        "$env:VULKAN_SDK\Bin32\vulkan-1.dll",
        "$env:VULKAN_SDK\Bin64\vulkan-1.dll"
    )
    
    foreach ($path in $paths) {
        if (Test-Path $path) {
            Write-Host "  [找到] $path" -ForegroundColor Green
            $foundDlls += $path
        }
    }
}

# 2. 检查常见的 Vulkan SDK 安装路径
Write-Host ""
Write-Host "检查常见的 Vulkan SDK 安装路径..." -ForegroundColor Yellow

$commonPaths = @(
    "C:\VulkanSDK",
    "D:\VulkanSDK",
    "C:\Program Files\Vulkan SDK",
    "C:\Program Files (x86)\Vulkan SDK"
)

foreach ($basePath in $commonPaths) {
    if (Test-Path $basePath) {
        Write-Host "  检查: $basePath" -ForegroundColor Gray
        
        # 查找所有版本目录
        $versionDirs = Get-ChildItem -Path $basePath -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '^\d+\.\d+\.\d+' }
        
        foreach ($versionDir in $versionDirs) {
            $dllPaths = @(
                "$($versionDir.FullName)\Bin\vulkan-1.dll",
                "$($versionDir.FullName)\vulkan-1.dll",
                "$($versionDir.FullName)\Bin32\vulkan-1.dll",
                "$($versionDir.FullName)\Bin64\vulkan-1.dll"
            )
            
            foreach ($dllPath in $dllPaths) {
                if (Test-Path $dllPath) {
                    Write-Host "    [找到] $dllPath" -ForegroundColor Green
                    $foundDlls += $dllPath
                }
            }
        }
    }
}

# 3. 检查系统目录
Write-Host ""
Write-Host "检查系统目录..." -ForegroundColor Yellow

$systemPaths = @(
    "$env:SystemRoot\System32\vulkan-1.dll",
    "$env:SystemRoot\SysWOW64\vulkan-1.dll"
)

foreach ($path in $systemPaths) {
    if (Test-Path $path) {
        Write-Host "  [找到] $path" -ForegroundColor Green
        $foundDlls += $path
    }
}

# 4. 检查 PATH 环境变量中的目录
Write-Host ""
Write-Host "检查 PATH 环境变量中的目录..." -ForegroundColor Yellow

$pathDirs = $env:PATH -split ';' | Where-Object { $_ -and (Test-Path $_) }

foreach ($dir in $pathDirs) {
    $dllPath = Join-Path $dir "vulkan-1.dll"
    if (Test-Path $dllPath) {
        Write-Host "  [找到] $dllPath" -ForegroundColor Green
        $foundDlls += $dllPath
    }
}

# 5. 检查项目目录
Write-Host ""
Write-Host "检查项目目录..." -ForegroundColor Yellow

$projectPaths = @(
    "$PSScriptRoot\external\vulkan\vulkan-1.dll",
    "$PSScriptRoot\external\vulkan\Bin\vulkan-1.dll",
    "$PSScriptRoot\build\bin\Debug\vulkan-1.dll",
    "$PSScriptRoot\build\bin\Release\vulkan-1.dll"
)

foreach ($path in $projectPaths) {
    if (Test-Path $path) {
        Write-Host "  [找到] $path" -ForegroundColor Green
        $foundDlls += $path
    }
}

# 总结
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($foundDlls.Count -gt 0) {
    Write-Host "找到 $($foundDlls.Count) 个 vulkan-1.dll 文件:" -ForegroundColor Green
    foreach ($dll in $foundDlls) {
        $fileInfo = Get-Item $dll
        Write-Host "  - $dll" -ForegroundColor White
        Write-Host "    大小: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Gray
        Write-Host "    修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "推荐使用第一个找到的文件（通常是最新的）" -ForegroundColor Yellow
    Write-Host "可以复制到: $PSScriptRoot\external\vulkan\" -ForegroundColor Yellow
} else {
    Write-Host "未找到 vulkan-1.dll!" -ForegroundColor Red
    Write-Host ""
    Write-Host "解决方案:" -ForegroundColor Yellow
    Write-Host "1. 安装 Vulkan SDK: https://vulkan.lunarg.com/sdk/home#windows" -ForegroundColor White
    Write-Host "2. 或者安装 Vulkan Runtime (仅运行时，不包含开发工具)" -ForegroundColor White
    Write-Host "3. 或者从显卡驱动中获取（NVIDIA/AMD/Intel 驱动通常包含 Vulkan Runtime）" -ForegroundColor White
}
Write-Host "========================================" -ForegroundColor Cyan
