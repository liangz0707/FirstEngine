# 在 Vulkan SDK 中查找 vulkan-1.dll

Write-Host "在 Vulkan SDK 中查找 vulkan-1.dll..." -ForegroundColor Cyan
Write-Host ""

$foundDlls = @()

# 1. 检查环境变量
if ($env:VULKAN_SDK) {
    Write-Host "检查 VULKAN_SDK: $env:VULKAN_SDK" -ForegroundColor Yellow
    
    $searchPaths = @(
        "$env:VULKAN_SDK\Bin",
        "$env:VULKAN_SDK\Bin32",
        "$env:VULKAN_SDK\Bin64",
        "$env:VULKAN_SDK",
        "$env:VULKAN_SDK\Runtime\Bin",
        "$env:VULKAN_SDK\Runtime\Bin32",
        "$env:VULKAN_SDK\Runtime\Bin64"
    )
    
    foreach ($path in $searchPaths) {
        $dllPath = Join-Path $path "vulkan-1.dll"
        if (Test-Path $dllPath) {
            Write-Host "  [找到] $dllPath" -ForegroundColor Green
            $foundDlls += $dllPath
        }
    }
    
    # 递归搜索（如果上面没找到）
    if ($foundDlls.Count -eq 0) {
        Write-Host "  递归搜索 $env:VULKAN_SDK..." -ForegroundColor Gray
        $allDlls = Get-ChildItem -Path $env:VULKAN_SDK -Filter "vulkan-1.dll" -Recurse -ErrorAction SilentlyContinue
        foreach ($dll in $allDlls) {
            Write-Host "  [找到] $($dll.FullName)" -ForegroundColor Green
            $foundDlls += $dll.FullName
        }
    }
}

# 2. 检查常见安装路径
Write-Host ""
Write-Host "检查常见安装路径..." -ForegroundColor Yellow

$commonBases = @(
    "C:\VulkanSDK",
    "D:\VulkanSDK",
    "C:\Program Files\Vulkan SDK",
    "C:\Program Files (x86)\Vulkan SDK"
)

foreach ($base in $commonBases) {
    if (Test-Path $base) {
        Write-Host "  检查: $base" -ForegroundColor Gray
        
        # 查找所有版本目录
        $versionDirs = Get-ChildItem -Path $base -Directory -ErrorAction SilentlyContinue | 
            Where-Object { $_.Name -match '^\d+\.\d+\.\d+' } | 
            Sort-Object Name -Descending
        
        foreach ($versionDir in $versionDirs) {
            Write-Host "    版本: $($versionDir.Name)" -ForegroundColor Gray
            
            $searchPaths = @(
                "$($versionDir.FullName)\Bin",
                "$($versionDir.FullName)\Bin32",
                "$($versionDir.FullName)\Bin64",
                "$($versionDir.FullName)",
                "$($versionDir.FullName)\Runtime\Bin",
                "$($versionDir.FullName)\Runtime\Bin32",
                "$($versionDir.FullName)\Runtime\Bin64"
            )
            
            foreach ($path in $searchPaths) {
                $dllPath = Join-Path $path "vulkan-1.dll"
                if (Test-Path $dllPath) {
                    Write-Host "      [找到] $dllPath" -ForegroundColor Green
                    $foundDlls += $dllPath
                }
            }
            
            # 如果还没找到，递归搜索这个版本目录
            if ($foundDlls.Count -eq 0 -or ($foundDlls | Where-Object { $_ -like "$($versionDir.FullName)*" }).Count -eq 0) {
                Write-Host "      递归搜索..." -ForegroundColor Gray
                $allDlls = Get-ChildItem -Path $versionDir.FullName -Filter "vulkan-1.dll" -Recurse -ErrorAction SilentlyContinue -Depth 3
                foreach ($dll in $allDlls) {
                    Write-Host "      [找到] $($dll.FullName)" -ForegroundColor Green
                    $foundDlls += $dll.FullName
                }
            }
        }
    }
}

# 3. 列出 SDK 目录结构（帮助理解）
Write-Host ""
Write-Host "Vulkan SDK 目录结构示例:" -ForegroundColor Yellow
Write-Host "  VulkanSDK\1.3.xxx\" -ForegroundColor White
Write-Host "    ├── Bin\          (64位可执行文件和DLL)" -ForegroundColor Gray
Write-Host "    ├── Bin32\         (32位可执行文件和DLL)" -ForegroundColor Gray
Write-Host "    ├── Bin64\         (64位可执行文件和DLL，某些版本)" -ForegroundColor Gray
Write-Host "    ├── Include\       (头文件)" -ForegroundColor Gray
Write-Host "    ├── Lib\           (库文件)" -ForegroundColor Gray
Write-Host "    └── Runtime\       (运行时文件，某些版本)" -ForegroundColor Gray
Write-Host ""

# 总结
Write-Host "========================================" -ForegroundColor Cyan
if ($foundDlls.Count -gt 0) {
    Write-Host "找到 $($foundDlls.Count) 个 vulkan-1.dll:" -ForegroundColor Green
    foreach ($dll in $foundDlls) {
        $fileInfo = Get-Item $dll
        Write-Host "  - $dll" -ForegroundColor White
        Write-Host "    大小: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor Gray
        Write-Host "    修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    }
    
    Write-Host ""
    Write-Host "推荐使用第一个找到的文件" -ForegroundColor Yellow
    Write-Host "可以复制到: $PSScriptRoot\external\vulkan\" -ForegroundColor Yellow
} else {
    Write-Host "未找到 vulkan-1.dll!" -ForegroundColor Red
    Write-Host ""
    Write-Host "可能的原因:" -ForegroundColor Yellow
    Write-Host "1. Vulkan SDK 安装不完整" -ForegroundColor White
    Write-Host "2. 新版本 SDK 结构有变化" -ForegroundColor White
    Write-Host "3. DLL 在不同的位置" -ForegroundColor White
    Write-Host ""
    Write-Host "解决方案:" -ForegroundColor Yellow
    Write-Host "1. 重新安装 Vulkan SDK，确保选择完整安装" -ForegroundColor White
    Write-Host "2. 检查 SDK 安装日志，确认所有组件都已安装" -ForegroundColor White
    Write-Host "3. 从 Vulkan Runtime 安装程序获取 DLL" -ForegroundColor White
    Write-Host "4. 从显卡驱动中获取（NVIDIA/AMD/Intel 驱动通常包含）" -ForegroundColor White
}
Write-Host "========================================" -ForegroundColor Cyan
