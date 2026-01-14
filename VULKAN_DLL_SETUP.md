# Vulkan DLL 设置指南

## 问题说明

如果遇到 `Error: GLFW reports Vulkan is not supported on this system!` 错误，通常是因为运行时找不到 `vulkan-1.dll`。

## 解决方案

### 方法 1: 使用本地 Vulkan SDK（推荐）

1. **安装 Vulkan SDK**:
   - 下载地址: https://vulkan.lunarg.com/sdk/home#windows
   - 安装到默认位置（如 `C:\VulkanSDK\1.3.xxx`）

2. **设置本地 Vulkan SDK**:
   ```powershell
   # 运行设置脚本（会自动复制 Vulkan SDK 到 external/vulkan）
   .\setup_external_deps.ps1
   ```
   
   或者手动复制：
   ```powershell
   # 从 Vulkan SDK 安装目录复制到项目
   xcopy /E /I "C:\VulkanSDK\1.3.xxx" "external\vulkan"
   ```

3. **验证 vulkan-1.dll 存在**:
   ```powershell
   # 检查文件是否存在
   Test-Path "external\vulkan\vulkan-1.dll"
   Test-Path "external\vulkan\Bin\vulkan-1.dll"
   ```

4. **重新生成 CMake 项目**:
   ```powershell
   # 删除旧的构建目录
   Remove-Item -Recurse -Force build
   
   # 重新生成
   mkdir build
   cd build
   cmake .. -G "Visual Studio 17 2022" -A x64
   ```

### 方法 2: 使用系统 Vulkan SDK

如果不想使用本地 Vulkan SDK，确保：

1. **设置环境变量**:
   ```powershell
   # 设置 VULKAN_SDK 环境变量
   $env:VULKAN_SDK = "C:\VulkanSDK\1.3.xxx"
   [System.Environment]::SetEnvironmentVariable("VULKAN_SDK", "C:\VulkanSDK\1.3.xxx", "User")
   ```

2. **确保 vulkan-1.dll 在 PATH 中**:
   - Vulkan SDK 安装程序通常会自动将 `Bin` 目录添加到 PATH
   - 或者手动将 `C:\VulkanSDK\1.3.xxx\Bin` 添加到系统 PATH

3. **验证**:
   ```powershell
   # 检查环境变量
   echo $env:VULKAN_SDK
   
   # 检查 DLL 是否存在
   Test-Path "$env:VULKAN_SDK\Bin\vulkan-1.dll"
   ```

### 方法 3: 手动复制 DLL 到输出目录

如果上述方法都不行，可以手动复制：

1. **找到 vulkan-1.dll**:
   - 通常在 `C:\VulkanSDK\1.3.xxx\Bin\vulkan-1.dll`
   - 或 `C:\VulkanSDK\1.3.xxx\vulkan-1.dll`

2. **复制到输出目录**:
   ```powershell
   # 复制到 Debug 输出目录
   Copy-Item "C:\VulkanSDK\1.3.xxx\Bin\vulkan-1.dll" "build\bin\Debug\"
   
   # 复制到 Release 输出目录
   Copy-Item "C:\VulkanSDK\1.3.xxx\Bin\vulkan-1.dll" "build\bin\Release\"
   ```

## CMake 自动复制

项目已配置为在构建时自动复制 `vulkan-1.dll` 到输出目录（如果找到的话）。CMake 会在以下位置查找：

1. `external/vulkan/vulkan-1.dll`
2. `external/vulkan/Bin/vulkan-1.dll`
3. `$VULKAN_SDK/Bin/vulkan-1.dll`
4. `$VULKAN_SDK/vulkan-1.dll`

## 验证设置

运行程序后，如果看到以下输出，说明设置成功：

```
GLFW Vulkan support: OK
GLFW required extensions (2):
  - VK_KHR_surface
  - VK_KHR_win32_surface
```

如果仍然失败，请检查：

1. **显卡驱动是否支持 Vulkan**:
   - NVIDIA: 需要 GeForce 600 系列或更新
   - AMD: 需要 GCN 架构或更新
   - Intel: 需要 Skylake 或更新

2. **驱动是否最新**:
   - 更新到最新的显卡驱动

3. **Vulkan Runtime 是否安装**:
   - 通常随显卡驱动一起安装
   - 或从 Vulkan SDK 安装程序安装

## 常见问题

### Q: 为什么需要 vulkan-1.dll？

A: `vulkan-1.dll` 是 Vulkan 加载器，GLFW 需要它来检测 Vulkan 支持并创建窗口表面。

### Q: 我已经安装了 Vulkan SDK，为什么还是找不到？

A: 确保：
- `VULKAN_SDK` 环境变量已设置
- 或者 `vulkan-1.dll` 在系统 PATH 中
- 或者已复制到 `external/vulkan` 目录

### Q: 可以只复制 vulkan-1.dll 而不复制整个 SDK 吗？

A: 可以，但需要确保：
- 头文件（`vulkan.h`）在编译时可用
- 库文件（`vulkan-1.lib`）在链接时可用
- DLL 在运行时可用

### Q: 构建时显示 "vulkan-1.dll not found" 警告怎么办？

A: 这是警告，不是错误。如果运行时仍然失败，请手动复制 DLL 到输出目录。
