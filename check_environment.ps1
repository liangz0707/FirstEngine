# FirstEngine 环境依赖检查脚本
# 运行此脚本检查所有必需的系统依赖

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "FirstEngine 环境依赖检查" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

$allOk = $true

# 检查 Python
Write-Host "`n[1] 检查 Python 3.x..." -ForegroundColor Yellow
$pythonFound = $false
$pythonPath = ""

# 检查系统 PATH 中的 Python
if (Get-Command python -ErrorAction SilentlyContinue) {
    $version = python --version 2>&1
    if ($version -match "Python 3\.") {
        Write-Host "  ✓ 找到: $version" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = (Get-Command python).Source
    } else {
        Write-Host "  ✗ 找到 Python 但不是 3.x: $version" -ForegroundColor Red
    }
}

# 检查默认配置路径
if (-not $pythonFound) {
    $defaultPath = "D:\Python3.9"
    if (Test-Path $defaultPath) {
        Write-Host "  ✓ 找到配置的 Python 路径: $defaultPath" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = $defaultPath
    }
}

# 检查环境变量
if (-not $pythonFound -and $env:PYTHON_ROOT_DIR) {
    if (Test-Path $env:PYTHON_ROOT_DIR) {
        Write-Host "  ✓ 找到环境变量 PYTHON_ROOT_DIR: $env:PYTHON_ROOT_DIR" -ForegroundColor Green
        $pythonFound = $true
        $pythonPath = $env:PYTHON_ROOT_DIR
    }
}

if (-not $pythonFound) {
    Write-Host "  ✗ 未找到 Python 3.x" -ForegroundColor Red
    Write-Host "    请安装 Python 3.9+ 或设置 PYTHON_ROOT_DIR 环境变量" -ForegroundColor Yellow
    $allOk = $false
} else {
    Write-Host "    路径: $pythonPath" -ForegroundColor Gray
}

# 检查 Vulkan SDK
Write-Host "`n[2] 检查 Vulkan SDK..." -ForegroundColor Yellow
if ($env:VULKAN_SDK) {
    if (Test-Path $env:VULKAN_SDK) {
        Write-Host "  ✓ 找到: $env:VULKAN_SDK" -ForegroundColor Green
        if (Test-Path "$env:VULKAN_SDK\Include\vulkan\vulkan.h") {
            Write-Host "    ✓ Vulkan 头文件存在" -ForegroundColor Green
        } else {
            Write-Host "    ✗ Vulkan 头文件不存在" -ForegroundColor Red
            $allOk = $false
        }
    } else {
        Write-Host "  ✗ VULKAN_SDK 环境变量指向不存在的路径" -ForegroundColor Red
        $allOk = $false
    }
} else {
    Write-Host "  ✗ 未设置 VULKAN_SDK 环境变量" -ForegroundColor Red
    Write-Host "    请安装 Vulkan SDK 并设置环境变量" -ForegroundColor Yellow
    Write-Host "    下载地址: https://www.vulkan.org/tools" -ForegroundColor Gray
    $allOk = $false
}

# 检查 CMake
Write-Host "`n[3] 检查 CMake..." -ForegroundColor Yellow
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $version = cmake --version | Select-Object -First 1
    Write-Host "  ✓ 找到: $version" -ForegroundColor Green
    
    # 检查版本
    $versionMatch = $version -match "version (\d+)\.(\d+)"
    if ($versionMatch) {
        $major = [int]$matches[1]
        $minor = [int]$matches[2]
        if ($major -gt 3 -or ($major -eq 3 -and $minor -ge 20)) {
            Write-Host "    ✓ 版本满足要求 (>= 3.20)" -ForegroundColor Green
        } else {
            Write-Host "    ✗ 版本过低，需要 >= 3.20" -ForegroundColor Red
            $allOk = $false
        }
    }
} else {
    Write-Host "  ✗ 未找到 CMake" -ForegroundColor Red
    Write-Host "    请安装 CMake 3.20 或更高版本" -ForegroundColor Yellow
    $allOk = $false
}

# 检查 Visual Studio 编译器
Write-Host "`n[4] 检查 Visual Studio 编译器..." -ForegroundColor Yellow
if (Get-Command cl -ErrorAction SilentlyContinue) {
    Write-Host "  ✓ 找到 Visual Studio 编译器" -ForegroundColor Green
    $clVersion = cl 2>&1 | Select-Object -First 1
    Write-Host "    $clVersion" -ForegroundColor Gray
} else {
    Write-Host "  ✗ 未找到 Visual Studio 编译器" -ForegroundColor Red
    Write-Host "    请使用 Visual Studio Developer Command Prompt" -ForegroundColor Yellow
    Write-Host "    或在 Visual Studio 中打开项目" -ForegroundColor Yellow
    $allOk = $false
}

# 检查 Git 子模块
Write-Host "`n[5] 检查 Git 子模块..." -ForegroundColor Yellow
$submodules = @("third_party/glfw", "third_party/glm", "third_party/spirv-cross", 
                "third_party/glslang", "third_party/pybind11", "third_party/stb", 
                "third_party/assimp")
$submodulesOk = $true
foreach ($submodule in $submodules) {
    if (Test-Path $submodule) {
        Write-Host "  ✓ $submodule" -ForegroundColor Green
    } else {
        Write-Host "  ✗ $submodule 未找到" -ForegroundColor Red
        $submodulesOk = $false
        $allOk = $false
    }
}
if (-not $submodulesOk) {
    Write-Host "    运行: git submodule update --init --recursive" -ForegroundColor Yellow
}

# 总结
Write-Host "`n========================================" -ForegroundColor Cyan
if ($allOk) {
    Write-Host "✓ 所有依赖检查通过！" -ForegroundColor Green
    Write-Host "  可以开始构建项目了" -ForegroundColor Green
} else {
    Write-Host "✗ 部分依赖缺失或配置不正确" -ForegroundColor Red
    Write-Host "  请根据上述提示修复问题" -ForegroundColor Yellow
    Write-Host "`n详细说明请查看 ENVIRONMENT.md" -ForegroundColor Cyan
}
Write-Host "========================================" -ForegroundColor Cyan
