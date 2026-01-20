# FirstEngine 环境变量和系统依赖说明

本文档列出了 FirstEngine 项目所需的所有系统环境变量和依赖项。

## 必需的系统依赖

### 1. Python 3.x
- **用途**: FirstEngine_Python 模块需要
- **推荐版本**: Python 3.9 或更高版本
- **安装路径**: 
  - 默认: 系统 PATH 中
  - 自定义: `D:\Python3.9` (已在 CMakeLists.txt 中配置)
- **环境变量**:
  - `PYTHON_ROOT_DIR`: Python 安装根目录（可选，如果不在 PATH 中）
- **验证方法**:
  ```bash
  python --version
  # 或
  python3 --version
  ```
- **如果未安装**: FirstEngine_Python 项目将出现在解决方案中，但无法编译

### 2. Vulkan SDK
- **用途**: Vulkan 图形 API
- **必需组件**: 
  - Vulkan SDK (包含头文件和库)
  - Vulkan 驱动程序
- **环境变量**:
  - `VULKAN_SDK`: Vulkan SDK 安装路径
  - `VK_SDK_PATH`: 替代环境变量（某些版本使用）
- **验证方法**:
  ```bash
  echo %VULKAN_SDK%
  # 或检查
  dir "%VULKAN_SDK%\Include\vulkan"
  ```
- **下载地址**: https://www.vulkan.org/tools

### 3. CMake
- **用途**: 构建系统
- **最低版本**: 3.20
- **环境变量**:
  - `CMAKE_HOME`: CMake 安装路径（可选）
  - CMake 应在系统 PATH 中
- **验证方法**:
  ```bash
  cmake --version
  ```

### 4. C++ 编译器
- **Windows**: Visual Studio 2019 或更高版本
- **环境变量**: 
  - Visual Studio 会自动设置必要的环境变量
  - 通过 Visual Studio Developer Command Prompt 运行 CMake

## 可选依赖（已集成到项目中）

以下依赖已作为 Git 子模块集成到 `third_party/` 目录，**不需要**单独安装：

- **GLFW**: 窗口管理（third_party/glfw）
- **GLM**: 数学库（third_party/glm）
- **SPIRV-Cross**: SPIR-V 转换器（third_party/spirv-cross）
- **glslang**: Shader 编译器（third_party/glslang）
- **pybind11**: Python 绑定（third_party/pybind11）
- **stb**: 图像加载库（third_party/stb）
- **assimp**: 模型加载库（third_party/assimp）

## 环境变量配置指南

### Windows 配置步骤

1. **设置 Python 路径（如果不在默认位置）**:
   ```powershell
   # 方法1: 设置环境变量
   [System.Environment]::SetEnvironmentVariable("PYTHON_ROOT_DIR", "D:\Python3.9", "User")
   
   # 方法2: 在 CMakeLists.txt 中已配置 D:\Python3.9，无需额外设置
   ```

2. **设置 Vulkan SDK 路径**:
   ```powershell
   # 如果 Vulkan SDK 未自动设置环境变量
   [System.Environment]::SetEnvironmentVariable("VULKAN_SDK", "C:\VulkanSDK\1.3.xxx", "User")
   ```

3. **验证环境变量**:
   ```powershell
   # 检查 Python
   python --version
   
   # 检查 Vulkan
   echo $env:VULKAN_SDK
   
   # 检查 CMake
   cmake --version
   ```

### 使用 Visual Studio Developer Command Prompt

推荐使用 Visual Studio Developer Command Prompt 来配置和构建项目，它会自动设置所有必要的环境变量。

## 快速检查脚本

创建 `check_environment.ps1` 来检查所有依赖：

```powershell
Write-Host "检查 FirstEngine 环境依赖..." -ForegroundColor Cyan

# 检查 Python
Write-Host "`n[Python]" -ForegroundColor Yellow
if (Get-Command python -ErrorAction SilentlyContinue) {
    $version = python --version 2>&1
    Write-Host "  找到: $version" -ForegroundColor Green
} else {
    Write-Host "  未找到 Python" -ForegroundColor Red
    Write-Host "  请安装 Python 3.9+ 或设置 PYTHON_ROOT_DIR" -ForegroundColor Yellow
}

# 检查 Vulkan
Write-Host "`n[Vulkan SDK]" -ForegroundColor Yellow
if ($env:VULKAN_SDK) {
    Write-Host "  找到: $env:VULKAN_SDK" -ForegroundColor Green
} else {
    Write-Host "  未设置 VULKAN_SDK" -ForegroundColor Red
    Write-Host "  请安装 Vulkan SDK 并设置环境变量" -ForegroundColor Yellow
}

# 检查 CMake
Write-Host "`n[CMake]" -ForegroundColor Yellow
if (Get-Command cmake -ErrorAction SilentlyContinue) {
    $version = cmake --version | Select-Object -First 1
    Write-Host "  找到: $version" -ForegroundColor Green
} else {
    Write-Host "  未找到 CMake" -ForegroundColor Red
}

# 检查 Visual Studio
Write-Host "`n[Visual Studio]" -ForegroundColor Yellow
if (Get-Command cl -ErrorAction SilentlyContinue) {
    Write-Host "  找到 Visual Studio 编译器" -ForegroundColor Green
} else {
    Write-Host "  未找到 Visual Studio 编译器" -ForegroundColor Red
    Write-Host "  请使用 Visual Studio Developer Command Prompt" -ForegroundColor Yellow
}

Write-Host "`n检查完成！" -ForegroundColor Cyan
```

## 常见问题

### Q: Python 未找到，但已安装
**A**: 
1. 确保 Python 在系统 PATH 中
2. 或设置 `PYTHON_ROOT_DIR` 环境变量指向 Python 安装目录
3. 或在 `src/Python/CMakeLists.txt` 中修改 `PYTHON_ROOT_DIR` 的默认值

### Q: Vulkan SDK 未找到
**A**: 
1. 下载并安装 Vulkan SDK
2. 安装程序通常会自动设置 `VULKAN_SDK` 环境变量
3. 如果未自动设置，手动添加环境变量

### Q: 如何避免依赖系统路径？
**A**: 
- 大部分依赖已作为子模块集成到项目中
- Python 和 Vulkan SDK 需要系统安装（文件较大，不适合打包）
- 可以通过设置环境变量或修改 CMakeLists.txt 中的路径来避免依赖系统 PATH

## 项目特定配置

### Python 路径配置
项目已配置默认 Python 路径为 `D:\Python3.9`。如果您的 Python 安装在其他位置：

1. **方法1**: 修改 `src/Python/CMakeLists.txt` 中的 `PYTHON_ROOT_DIR` 默认值
2. **方法2**: 设置环境变量 `PYTHON_ROOT_DIR`
3. **方法3**: 在 CMake 配置时指定：
   ```bash
   cmake .. -DPYTHON_ROOT_DIR="C:\Your\Python\Path"
   ```

## 总结

**必须安装的系统依赖**:
- ✅ Python 3.9+ (已配置 D:\Python3.9)
- ✅ Vulkan SDK
- ✅ CMake 3.20+
- ✅ Visual Studio 2019+

**已集成到项目中的依赖**（无需单独安装）:
- ✅ GLFW, GLM, SPIRV-Cross, glslang, pybind11, stb, assimp

**环境变量**:
- `VULKAN_SDK`: Vulkan SDK 路径
- `PYTHON_ROOT_DIR`: Python 安装路径（可选，已配置默认值）
