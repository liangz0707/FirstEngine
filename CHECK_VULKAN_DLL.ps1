# 检查 vulkan-1.dll 是否可用

param(
    [string]$DllPath = ""
)

Write-Host "检查 vulkan-1.dll 可用性..." -ForegroundColor Cyan
Write-Host ""

if ([string]::IsNullOrEmpty($DllPath)) {
    # 如果没有指定路径，查找 DLL
    Write-Host "查找 vulkan-1.dll..." -ForegroundColor Yellow
    
    $foundDlls = @()
    
    # 检查系统目录
    $systemDll = "C:\Windows\System32\vulkan-1.dll"
    if (Test-Path $systemDll) {
        $foundDlls += $systemDll
    }
    
    # 检查 VULKAN_SDK
    if ($env:VULKAN_SDK) {
        $sdkDll = "$env:VULKAN_SDK\Bin\vulkan-1.dll"
        if (Test-Path $sdkDll) {
            $foundDlls += $sdkDll
        }
    }
    
    # 检查常见路径
    $commonPaths = @(
        "C:\VulkanSDK\1.3.275.0\Bin\vulkan-1.dll",
        "C:\VulkanSDK\1.3.250.0\Bin\vulkan-1.dll",
        "C:\VulkanSDK\1.3.224.1\Bin\vulkan-1.dll"
    )
    
    foreach ($path in $commonPaths) {
        if (Test-Path $path) {
            $foundDlls += $path
        }
    }
    
    if ($foundDlls.Count -eq 0) {
        Write-Host "未找到 vulkan-1.dll!" -ForegroundColor Red
        exit 1
    }
    
    # 使用第一个找到的 DLL
    $DllPath = $foundDlls[0]
    Write-Host "使用: $DllPath" -ForegroundColor Green
    Write-Host ""
}

if (-not (Test-Path $DllPath)) {
    Write-Host "错误: 文件不存在: $DllPath" -ForegroundColor Red
    exit 1
}

# 检查文件信息
Write-Host "文件信息:" -ForegroundColor Yellow
$fileInfo = Get-Item $DllPath
Write-Host "  路径: $($fileInfo.FullName)" -ForegroundColor White
Write-Host "  大小: $([math]::Round($fileInfo.Length / 1KB, 2)) KB" -ForegroundColor White
Write-Host "  修改时间: $($fileInfo.LastWriteTime)" -ForegroundColor White

# 检查架构（32位 vs 64位）
Write-Host ""
Write-Host "检查 DLL 架构..." -ForegroundColor Yellow

try {
    # 使用 dumpbin 检查（如果可用）
    $dumpbinPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\*\VC\Tools\MSVC\*\bin\Hostx64\x64\dumpbin.exe"
    $dumpbin = Get-ChildItem -Path $dumpbinPath -ErrorAction SilentlyContinue | Select-Object -First 1
    
    if ($dumpbin) {
        Write-Host "  使用 dumpbin 检查..." -ForegroundColor Gray
        $output = & $dumpbin.FullName /headers $DllPath 2>&1
        if ($output -match "machine \(x64\)") {
            Write-Host "  架构: 64-bit (x64)" -ForegroundColor Green
        } elseif ($output -match "machine \(x86\)") {
            Write-Host "  架构: 32-bit (x86)" -ForegroundColor Yellow
            Write-Host "  警告: 这是 32 位 DLL，64 位程序无法使用！" -ForegroundColor Red
        } else {
            Write-Host "  架构: 未知" -ForegroundColor Yellow
        }
    } else {
        Write-Host "  dumpbin 不可用，跳过架构检查" -ForegroundColor Gray
    }
} catch {
    Write-Host "  无法检查架构: $_" -ForegroundColor Yellow
}

# 检查依赖
Write-Host ""
Write-Host "检查 DLL 依赖..." -ForegroundColor Yellow

try {
    # 使用 dumpbin 检查依赖
    if ($dumpbin) {
        $output = & $dumpbin.FullName /dependents $DllPath 2>&1
        $dependencies = $output | Where-Object { $_ -match "^\s+\S+\.(dll|DLL)" }
        
        if ($dependencies) {
            Write-Host "  依赖的 DLL:" -ForegroundColor White
            foreach ($dep in $dependencies) {
                $depName = $dep.Trim()
                Write-Host "    - $depName" -ForegroundColor Gray
                
                # 检查依赖是否存在
                $depPath = Join-Path $env:SystemRoot "System32\$depName"
                if (Test-Path $depPath) {
                    Write-Host "      [找到] $depPath" -ForegroundColor Green
                } else {
                    Write-Host "      [缺失] $depName" -ForegroundColor Red
                }
            }
        } else {
            Write-Host "  无外部依赖" -ForegroundColor Green
        }
    } else {
        Write-Host "  dumpbin 不可用，跳过依赖检查" -ForegroundColor Gray
    }
} catch {
    Write-Host "  无法检查依赖: $_" -ForegroundColor Yellow
}

# 尝试加载 DLL
Write-Host ""
Write-Host "尝试加载 DLL..." -ForegroundColor Yellow

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public class DllLoader {
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
    public static extern IntPtr LoadLibrary(string lpFileName);
    
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern bool FreeLibrary(IntPtr hModule);
    
    [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);
}
"@

try {
    $hModule = [DllLoader]::LoadLibrary($DllPath)
    
    if ($hModule -ne [IntPtr]::Zero) {
        Write-Host "  DLL 加载成功" -ForegroundColor Green
        
        # 检查是否导出 vkGetInstanceProcAddr
        $procAddr = [DllLoader]::GetProcAddress($hModule, "vkGetInstanceProcAddr")
        
        if ($procAddr -ne [IntPtr]::Zero) {
            Write-Host "  导出 vkGetInstanceProcAddr: 是" -ForegroundColor Green
            Write-Host "  DLL 看起来是有效的 Vulkan 加载器" -ForegroundColor Green
        } else {
            Write-Host "  导出 vkGetInstanceProcAddr: 否" -ForegroundColor Red
            Write-Host "  警告: 这个 DLL 不是有效的 Vulkan 加载器！" -ForegroundColor Red
        }
        
        [DllLoader]::FreeLibrary($hModule) | Out-Null
    } else {
        $errorCode = [System.Runtime.InteropServices.Marshal]::GetLastWin32Error()
        Write-Host "  DLL 加载失败" -ForegroundColor Red
        Write-Host "  错误代码: $errorCode" -ForegroundColor Red
        
        switch ($errorCode) {
            126 { Write-Host "  错误: 找不到指定的模块（缺少依赖）" -ForegroundColor Red }
            193 { Write-Host "  错误: 不是有效的 Win32 应用程序（架构不匹配）" -ForegroundColor Red }
            default { Write-Host "  错误: 未知错误" -ForegroundColor Red }
        }
    }
} catch {
    Write-Host "  加载失败: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "建议:" -ForegroundColor Yellow
Write-Host "1. 如果 DLL 架构不匹配，请下载正确架构的版本" -ForegroundColor White
Write-Host "2. 如果缺少依赖，请安装 Visual C++ Redistributable" -ForegroundColor White
Write-Host "3. 如果 DLL 无效，请从 Vulkan SDK 重新获取" -ForegroundColor White
Write-Host "========================================" -ForegroundColor Cyan
