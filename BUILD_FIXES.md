# 编译问题修复记录

## 已修复的问题

### 1. VulkanShaderModule::GetStage() 返回类型
**问题**: 头文件中返回类型声明为 `ShaderStage`，应该是 `RHI::ShaderStage`
**修复**: `include/FirstEngine/Device/VulkanRHIWrappers.h`
```cpp
// 修复前
ShaderStage GetStage() const override { return m_Stage; }

// 修复后
RHI::ShaderStage GetStage() const override { return m_Stage; }
```

### 2. VulkanSwapchain 的所有权问题
**问题**: `m_Swapchain` 使用 `unique_ptr`，但 `Swapchain` 对象由 `VulkanRenderer` 拥有
**修复**: `include/FirstEngine/Device/VulkanRHIWrappers.h`
```cpp
// 修复前
std::unique_ptr<class Swapchain> m_Swapchain;

// 修复后
class Swapchain* m_Swapchain; // Raw pointer, owned by VulkanRenderer
```

### 3. Application CMakeLists.txt 项目定义
**问题**: 子目录重复定义 `project(FirstEngine)`
**修复**: `src/Application/CMakeLists.txt`
- 移除了重复的 `project()` 定义
- 使用直接目标名称 `FirstEngine` 替代 `${PROJECT_NAME}`

### 4. RHI CMakeLists.txt 源文件
**问题**: `RHI_SOURCES` 列表为空
**修复**: `src/RHI/CMakeLists.txt`
- 添加了 `RHI.cpp` 源文件

## 编译建议

如果仍有编译错误，请：

1. **清理构建目录**：
```powershell
cd build
Remove-Item -Recurse -Force *
cd ..
```

2. **重新配置和编译**：
```powershell
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

3. **查看详细错误信息**：
```powershell
cmake --build . --config Release --verbose
```

## 常见编译错误

### 链接错误
- 确保所有 DLL 文件都正确链接
- 检查 `target_link_libraries` 配置

### 未解析的外部符号
- 检查是否有遗漏的源文件
- 检查命名空间是否正确

### 类型不匹配
- 检查返回类型是否匹配接口定义
- 检查参数类型是否匹配
