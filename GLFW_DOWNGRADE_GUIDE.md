# GLFW 降级指南

## 当前情况

- **当前 GLFW 版本**: 3.5.0（最新版本）
- **问题**: 可能与某些 Vulkan Runtime 不兼容
- **建议版本**: GLFW 3.3.x 或 3.4.x（更稳定，兼容性更好）

## GLFW_INCLUDE_VULKAN 宏的影响

`GLFW_INCLUDE_VULKAN` 宏的作用：
- 让 GLFW 头文件自动包含 `<vulkan/vulkan.h>`
- 这样就不需要手动 `#include <vulkan/vulkan.h>`
- **不影响** `glfwVulkanSupported()` 的行为
- **不影响** DLL 加载逻辑

**结论**: 这个宏不是问题的原因，可以保留。

## 降级方案

### 方案 1: 使用 Git 子模块切换到旧版本（推荐）

```powershell
# 1. 进入 GLFW 目录
cd third_party\glfw

# 2. 查看可用的标签（版本）
git tag | Select-String "3\.[34]\."

# 3. 切换到稳定版本（例如 3.3.8 或 3.4.0）
git checkout 3.3.8
# 或
git checkout 3.4.0

# 4. 返回项目根目录
cd ..\..

# 5. 重新生成 CMake 项目
Remove-Item -Recurse -Force build
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

### 方案 2: 手动下载并替换

1. **下载旧版本**:
   - GLFW 3.3.8: https://github.com/glfw/glfw/releases/tag/3.3.8
   - GLFW 3.4.0: https://github.com/glfw/glfw/releases/tag/3.4.0

2. **替换文件**:
   ```powershell
   # 备份当前版本
   Rename-Item third_party\glfw third_party\glfw_backup
   
   # 解压下载的版本到 third_party\glfw
   # (手动解压或使用 Expand-Archive)
   ```

3. **重新生成项目**

### 方案 3: 使用预编译的二进制版本

1. **下载预编译版本**:
   - Windows 64-bit: https://github.com/glfw/glfw/releases
   - 选择 3.3.x 或 3.4.x 版本的 Windows 二进制包

2. **配置 CMakeLists.txt**:
   ```cmake
   # 在 CMakeLists.txt 中，注释掉 add_subdirectory
   # add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw glfw)
   
   # 改为使用预编译版本
   set(GLFW_DIR "path/to/glfw-3.3.8")
   find_package(glfw3 REQUIRED)
   ```

## 推荐的稳定版本

### GLFW 3.3.8（推荐）
- **优点**: 非常稳定，广泛使用
- **Vulkan 支持**: 完整支持
- **兼容性**: 与大多数 Vulkan Runtime 兼容

### GLFW 3.4.0
- **优点**: 较新，但比 3.5.0 稳定
- **Vulkan 支持**: 完整支持
- **兼容性**: 良好

## 快速降级脚本

创建 `downgrade_glfw.ps1`:

```powershell
param(
    [string]$Version = "3.3.8"
)

Write-Host "降级 GLFW 到版本 $Version..." -ForegroundColor Cyan

cd third_party\glfw

# 检查当前版本
$currentVersion = git describe --tags
Write-Host "当前版本: $currentVersion" -ForegroundColor Yellow

# 切换到指定版本
Write-Host "切换到版本 $Version..." -ForegroundColor Yellow
git checkout $Version

if ($LASTEXITCODE -eq 0) {
    Write-Host "成功切换到 GLFW $Version" -ForegroundColor Green
    Write-Host ""
    Write-Host "下一步:" -ForegroundColor Yellow
    Write-Host "1. 删除 build 目录" -ForegroundColor White
    Write-Host "2. 重新运行 cmake" -ForegroundColor White
} else {
    Write-Host "切换失败，请检查版本号是否正确" -ForegroundColor Red
    Write-Host "可用版本: $(git tag | Select-String '3\.[34]\.')" -ForegroundColor Yellow
}

cd ..\..
```

使用方法：
```powershell
.\downgrade_glfw.ps1 -Version "3.3.8"
```

## 验证降级

降级后，检查版本：

```powershell
cd third_party\glfw
git describe --tags
# 应该显示类似: 3.3.8
```

## 注意事项

1. **Git 子模块状态**: 如果 GLFW 是 Git 子模块，降级后可能需要更新 `.gitmodules`
2. **API 兼容性**: GLFW 3.3.x 和 3.4.x 的 API 与 3.5.0 基本兼容
3. **重新编译**: 降级后必须重新编译整个项目

## 如果降级后仍有问题

如果降级后仍然无法使用系统的 `vulkan-1.dll`，问题可能是：

1. **DLL 本身的问题**:
   - 架构不匹配（32位 vs 64位）
   - DLL 损坏或不完整
   - 版本过旧

2. **解决方案**:
   - 使用 Vulkan SDK 中的 DLL（推荐）
   - 将 DLL 复制到可执行文件目录
   - 检查 DLL 的依赖是否完整

## 关于 GLFW_INCLUDE_VULKAN

**这个宏可以保留**，它只是让代码更简洁，不影响功能。

如果遇到编译错误，可以：
1. 移除 `#define GLFW_INCLUDE_VULKAN`
2. 手动添加 `#include <vulkan/vulkan.h>`
