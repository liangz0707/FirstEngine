# 循环依赖修复总结

## 问题

CMake 报错：共享库参与循环依赖

```
CMake Error: The inter-target dependency graph contains the following strongly connected component (cycle):
  "FirstEngine_Core" of type SHARED_LIBRARY
    depends on "FirstEngine_Editor" (weak)
    depends on "FirstEngine_Device" (weak)
  "FirstEngine_Device" of type STATIC_LIBRARY
    depends on "FirstEngine_Core" (weak)
  "FirstEngine_Editor" of type STATIC_LIBRARY
    depends on "FirstEngine_Core" (weak)
At least one of these targets is not a STATIC_LIBRARY.  Cyclic dependencies are allowed only among static libraries.
```

## 原因

- `FirstEngine_Core` (SHARED) 链接到 `FirstEngine_Editor` (STATIC) 和 `FirstEngine_Device` (STATIC)
- `FirstEngine_Editor` (STATIC) 链接到 `FirstEngine_Core` (SHARED)
- `FirstEngine_Device` (STATIC) 链接到 `FirstEngine_Core` (SHARED)
- CMake **不允许共享库参与任何形式的循环依赖**，即使是通过静态库

## 解决方案

采用两步修复：

### 1. 将 `FirstEngine_Device` 改为静态库（STATIC）

### 修改内容

1. **`src/Device/CMakeLists.txt`**：
   - 将 `add_library(FirstEngine_Device SHARED ...)` 改为 `add_library(FirstEngine_Device STATIC ...)`
   - 移除 DLL 复制命令（静态库不需要）
   - 移除 `WINDOWS_EXPORT_ALL_SYMBOLS` 属性

2. **`include/FirstEngine/Device/Export.h`**：
   - 将 `FE_DEVICE_API` 在 Windows 上定义为空（静态库不需要导出/导入符号）

### 2. 将 `RenderDocHelper` 实现移到头文件中

**问题**：`FirstEngine_Editor` 需要 `RenderDocHelper`，但链接 `FirstEngine_Core` 会造成循环依赖。

**解决方案**：将 `RenderDocHelper` 的实现移到头文件中，使用 `inline static` 定义静态成员变量。

**修改内容**：

1. **`include/FirstEngine/Core/RenderDoc.h`**：
   - 将 `RenderDocHelper` 的所有方法实现移到头文件中（内联实现）
   - 使用 `inline static RENDERDOC_API_1_4_2* s_RenderDocAPI = nullptr;` 定义静态成员变量

2. **`src/Core/CMakeLists.txt`**：
   - 从源文件列表中移除 `RenderDoc.cpp`（实现已在头文件中）

3. **`src/Editor/CMakeLists.txt`**：
   - 移除对 `FirstEngine_Core` 的链接（只需要包含头文件）

## 依赖关系（修复后）

```
FirstEngine_Core (SHARED) 
  └─> FirstEngine_Editor (STATIC) ✅ 允许
      └─> FirstEngine_Device (STATIC) ✅ 允许
      └─> FirstEngine_Renderer (STATIC) ✅ 允许
      └─> FirstEngine_Resources (STATIC) ✅ 允许
      └─> FirstEngine_RHI ✅ 允许
      └─> FirstEngine_Shader ✅ 允许

FirstEngine_Device (STATIC)
  └─> FirstEngine_Core (SHARED) ✅ 允许（静态库可以链接共享库）

FirstEngine_Editor (STATIC)
  └─> FirstEngine_Device (STATIC) ✅ 允许
  └─> FirstEngine_Renderer (STATIC) ✅ 允许
  └─> FirstEngine_Resources (STATIC) ✅ 允许
  └─> FirstEngine_RHI ✅ 允许
  └─> FirstEngine_Shader ✅ 允许
  └─> FirstEngine_Core (SHARED) ❌ 不链接（RenderDocHelper 是头文件实现）
```

## CMake 依赖规则

CMake 允许的依赖关系：
- ✅ 共享库 → 静态库
- ✅ 静态库 → 共享库
- ✅ 静态库 → 静态库（包括循环依赖）
- ❌ 共享库 → 共享库（不允许循环依赖）

## 为什么这样修复？

1. **FirstEngine_Core 必须是共享库**：
   - C# 编辑器需要通过 P/Invoke 调用 `FirstEngine_Core.dll` 中的函数
   - 如果改为静态库，C# 无法使用

2. **FirstEngine_Device 可以改为静态库**：
   - `FirstEngine_Device` 不需要单独作为 DLL
   - 它会被链接到 `FirstEngine_Core.dll` 中
   - 静态库可以链接到共享库，不会造成问题

3. **循环依赖解决**：
   - `FirstEngine_Core` (SHARED) → `FirstEngine_Editor` (STATIC) ✅
   - `FirstEngine_Editor` (STATIC) → `FirstEngine_Device` (STATIC) ✅
   - `FirstEngine_Device` (STATIC) → `FirstEngine_Core` (SHARED) ✅
   - `FirstEngine_Editor` (STATIC) → `FirstEngine_Core` (SHARED) ❌ 不链接（使用头文件实现）

## 验证

修复后，CMake 配置应该能够成功：
- ✅ 没有循环依赖错误
- ✅ `FirstEngine_Device` 作为静态库链接到 `FirstEngine_Core.dll`
- ✅ `FirstEngine_Editor` 作为静态库链接到 `FirstEngine_Core.dll`
- ✅ `RenderDocHelper` 作为头文件实现，`FirstEngine_Editor` 不需要链接 `FirstEngine_Core`
- ✅ C# 编辑器可以正常使用 `FirstEngine_Core.dll`

## 技术细节

### inline static 成员变量（C++17）

使用 `inline static` 可以在头文件中定义静态成员变量，避免需要在 `.cpp` 文件中单独定义：

```cpp
class RenderDocHelper {
private:
    inline static RENDERDOC_API_1_4_2* s_RenderDocAPI = nullptr;
};
```

这样，每个包含 `RenderDoc.h` 的编译单元都会共享同一个静态成员变量实例，避免了重复定义错误。
