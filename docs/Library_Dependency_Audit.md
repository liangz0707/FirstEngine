# 库依赖和配置审计报告

本文档记录了库引用、循环依赖、导入导出和CMake配置的检查结果。

## 执行摘要

### ✅ 正确的配置

1. **循环依赖处理**：
   - `FirstEngine_Device` 设为 STATIC，避免与 `FirstEngine_Core` 的循环依赖
   - `FirstEngine_Renderer` 和 `FirstEngine_Resources` 都设为 STATIC，允许互相依赖
   - `Window.h` 和 `RenderDoc.h` 实现移到头文件，避免链接依赖

2. **导出宏**：
   - `FirstEngine_Device` (STATIC) - Export.h 正确使用空定义
   - `FirstEngine_Resources` (STATIC) - Export.h 正确使用空定义
   - `FirstEngine_Core` (SHARED) - Export.h 正确使用 `__declspec(dllexport/dllimport)`
   - `FirstEngine_RHI` (SHARED) - Export.h 正确使用 `__declspec(dllexport/dllimport)`
   - `FirstEngine_Shader` (SHARED) - Export.h 正确使用 `__declspec(dllexport/dllimport)`

### ✅ 已修复的问题

1. **静态库导出宏错误**：
   - ✅ `FirstEngine_Renderer` (STATIC) 的 Export.h 已修复，现在使用空定义（与 Device 和 Resources 一致）

2. **CMake DLL 复制配置**：
   - ✅ `Application/CMakeLists.txt` 已修复，移除了静态库的 DLL 复制命令

### ⚠️ 可选优化

3. **CMake配置冗余**：
   - 静态库设置了 `*_EXPORTS` 宏，这是不必要的（静态库不需要导出符号）
   - 但保留也不影响功能，可以作为未来兼容性（如果将来改为共享库）

### ✅ 已验证的配置

4. **头文件包含检查**：
   - ✅ Resources 和 Renderer 之间的头文件包含关系正确
   - ✅ 使用前向声明避免了循环包含

---

## 详细问题分析

### 问题 1: FirstEngine_Renderer 导出宏错误

**位置**: `include/FirstEngine/Renderer/Export.h`

**修复前代码**:
```cpp
#ifdef _WIN32
    #ifdef FirstEngine_Renderer_EXPORTS
        #define FE_RENDERER_API __declspec(dllexport)
    #else
        #define FE_RENDERER_API __declspec(dllimport)
    #endif
#else
    #define FE_RENDERER_API
#endif
```

**问题**: `FirstEngine_Renderer` 是 **STATIC** 库（见 `src/Renderer/CMakeLists.txt:87`），静态库不需要导出符号。使用 `__declspec(dllimport)` 可能导致链接错误。

**✅ 已修复代码**（与 `FirstEngine_Device/Export.h` 一致）:
```cpp
#ifdef _WIN32
    // FirstEngine_Renderer is a STATIC library, so we don't need __declspec(dllexport/dllimport)
    // Static libraries are linked directly into the executable/DLL, so symbols don't need to be exported
    #define FE_RENDERER_API
#else
    #define FE_RENDERER_API
#endif
```

**修复状态**: ✅ 已完成

---

### 问题 2: Application CMakeLists.txt DLL 复制配置

**位置**: `src/Application/CMakeLists.txt:50-56`

**修复前代码**:
```cmake
add_custom_command(TARGET FirstEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:FirstEngine_Core>
    $<TARGET_FILE:FirstEngine_Device>  # ❌ 静态库没有 DLL
    $<TARGET_FILE:FirstEngine_Shader>
    $<TARGET_FILE:FirstEngine_RHI>
    $<TARGET_FILE_DIR:FirstEngine>
)
```

**问题**: `FirstEngine_Device` 是静态库，没有 DLL 文件，复制命令会失败或产生警告。

**✅ 已修复代码**:
```cmake
add_custom_command(TARGET FirstEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:FirstEngine_Core>
    $<TARGET_FILE:FirstEngine_Shader>
    $<TARGET_FILE:FirstEngine_RHI>
    $<TARGET_FILE_DIR:FirstEngine>
)
```

**修复状态**: ✅ 已完成

---

### 问题 3: CMake 中静态库的 EXPORTS 宏设置（可选优化）

**位置**: 
- `src/Device/CMakeLists.txt:7, 88`
- `src/Renderer/CMakeLists.txt:7, 127`
- `src/Resources/CMakeLists.txt:7, 183`

**当前代码**:
```cmake
# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Device_EXPORTS)  # 静态库不需要
endif()

# ...

if(MSVC)
    target_compile_definitions(FirstEngine_Device PUBLIC FirstEngine_Device_EXPORTS)  # 静态库不需要
endif()
```

**问题**: 静态库不需要导出符号，设置 `*_EXPORTS` 宏是冗余的。

**建议**: 
- 可以保留（不影响功能，只是冗余）
- 或者移除以提高代码清晰度

**注意**: 如果将来将静态库改为共享库，需要这些宏，所以保留也可以作为未来兼容性。

---

### 问题 3: 头文件包含关系检查

#### Resources ↔ Renderer 循环依赖

**依赖关系**:
- `FirstEngine_Resources` (STATIC) 依赖 `FirstEngine_Renderer` (STATIC) - PRIVATE 链接
- `FirstEngine_Renderer` (STATIC) 依赖 `FirstEngine_Resources` (STATIC) - PRIVATE 链接

**头文件包含**:
- `MaterialResource.h` 使用前向声明，不直接包含 Renderer 头文件 ✅
- `ShadingMaterial.h` 使用前向声明 `MaterialResource` ✅
- `ModelComponent.cpp` 包含 Renderer 头文件（实现文件，可以）✅

**结论**: 头文件包含关系正确，使用前向声明避免了循环包含。

---

## 库类型总结

| 模块 | 类型 | 导出宏 | 状态 |
|-----|------|--------|------|
| **FirstEngine_Core** | SHARED | `FE_CORE_API` (dllexport/dllimport) | ✅ 正确 |
| **FirstEngine_Device** | STATIC | `FE_DEVICE_API` (空定义) | ✅ 正确 |
| **FirstEngine_Renderer** | STATIC | `FE_RENDERER_API` (dllexport/dllimport) | ❌ **错误** |
| **FirstEngine_Resources** | STATIC | `FE_RESOURCES_API` (空定义) | ✅ 正确 |
| **FirstEngine_RHI** | SHARED | `FE_RHI_API` (dllexport/dllimport) | ✅ 正确 |
| **FirstEngine_Shader** | SHARED | `FE_SHADER_API` (dllexport/dllimport) | ✅ 正确 |
| **FirstEngine_Python** | SHARED | `FE_PYTHON_API` (dllexport/dllimport) | ✅ 正确 |
| **FirstEngine_Editor** | STATIC | 无（静态库，不需要） | ✅ 正确 |

---

## 依赖关系图

```
FirstEngine (EXE)
    ├─ FirstEngine_Core (SHARED)
    │   ├─ FirstEngine_Device (STATIC) [PRIVATE]
    │   ├─ FirstEngine_Renderer (STATIC) [PRIVATE]
    │   ├─ FirstEngine_Resources (STATIC) [PRIVATE]
    │   ├─ FirstEngine_RHI (SHARED) [PRIVATE]
    │   └─ FirstEngine_Shader (SHARED) [PRIVATE]
    │
    ├─ FirstEngine_Device (STATIC)
    │   ├─ FirstEngine_Shader (SHARED) [PUBLIC]
    │   ├─ FirstEngine_Resources (STATIC) [PUBLIC]
    │   └─ FirstEngine_RHI (SHARED) [PUBLIC]
    │
    ├─ FirstEngine_Renderer (STATIC)
    │   ├─ FirstEngine_RHI (SHARED) [PUBLIC]
    │   ├─ FirstEngine_Resources (STATIC) [PRIVATE] ⚠️ 循环依赖
    │   └─ FirstEngine_Shader (SHARED) [PRIVATE]
    │
    ├─ FirstEngine_Resources (STATIC)
    │   ├─ FirstEngine_Renderer (STATIC) [PRIVATE] ⚠️ 循环依赖
    │   └─ assimp (第三方)
    │
    ├─ FirstEngine_RHI (SHARED)
    │   └─ (无依赖)
    │
    └─ FirstEngine_Shader (SHARED)
        └─ glslang, spirv-cross (第三方)
```

**循环依赖处理**:
- ✅ `FirstEngine_Renderer` ↔ `FirstEngine_Resources` 都设为 STATIC，允许循环依赖
- ✅ 使用 PRIVATE 链接，避免传播依赖
- ✅ 头文件使用前向声明，避免循环包含

---

## 修复建议

### ✅ 修复 1: 修正 FirstEngine_Renderer 导出宏（已完成）

**文件**: `include/FirstEngine/Renderer/Export.h`

**✅ 已修复**:
```cpp
#pragma once

#ifdef _WIN32
    // FirstEngine_Renderer is a STATIC library, so we don't need __declspec(dllexport/dllimport)
    // Static libraries are linked directly into the executable/DLL, so symbols don't need to be exported
    #define FE_RENDERER_API
#else
    #define FE_RENDERER_API
#endif
```

**理由**: 与 `FirstEngine_Device` 和 `FirstEngine_Resources` 保持一致，静态库不需要导出符号。

---

### ✅ 修复 2: Application CMakeLists.txt DLL 复制配置（已完成）

**文件**: `src/Application/CMakeLists.txt`

**✅ 已修复**: 移除了 `FirstEngine_Device` 的 DLL 复制命令（静态库没有 DLL）

---

### 💡 优化 3: 清理 CMake 中的冗余 EXPORTS 宏（可选）

**文件**: 
- `src/Device/CMakeLists.txt`
- `src/Renderer/CMakeLists.txt`
- `src/Resources/CMakeLists.txt`

**修改**: 移除或注释掉静态库的 `*_EXPORTS` 宏设置

**注意**: 这是可选的，保留也不影响功能。如果将来需要将静态库改为共享库，这些宏会有用。

---

## 其他检查项

### ✅ EditorAPI.def 文件

**位置**: `src/Editor/EditorAPI.def`

**状态**: ✅ 正确配置
- 在 `src/Core/CMakeLists.txt:48-55` 中正确使用
- 显式导出 EditorAPI 函数，比 `__declspec(dllexport)` 更可靠

---

### ✅ 链接顺序

**检查**: `src/Application/CMakeLists.txt:28-36`

**状态**: ✅ 正确
- 链接顺序合理：Core → Device → Renderer → Resources → RHI → Shader
- 静态库在共享库之前链接（虽然现代链接器通常能处理）

---

### ✅ DLL 复制配置（已修复）

**检查**: `src/Application/CMakeLists.txt:50-56`

**修复前**: 尝试复制 `FirstEngine_Device` DLL，但它是静态库

**✅ 已修复**: 移除了 `FirstEngine_Device` 的复制命令（静态库没有 DLL）

---

## 总结

### ✅ 已修复的问题

1. ✅ **FirstEngine_Renderer Export.h** - 已修复，静态库现在使用空定义
2. ✅ **Application CMakeLists.txt** - 已修复，移除了静态库的 DLL 复制命令

### 可选优化

3. 💡 **CMake EXPORTS 宏** - 清理静态库的冗余宏设置（可选，不影响功能）

### 正确的配置

- ✅ 循环依赖处理
- ✅ 头文件前向声明
- ✅ 库类型选择（静态/共享）
- ✅ EditorAPI.def 配置
- ✅ 导出宏配置（已修复）

---

## 修复状态

### ✅ 已完成的修复

1. **FirstEngine_Renderer/Export.h** - 已修复为静态库正确的空定义
2. **Application/CMakeLists.txt** - 已移除 `FirstEngine_Device` 的 DLL 复制（静态库没有 DLL）

### 待处理（可选）

3. **CMake EXPORTS 宏** - 可以清理但非必需（保留有助于未来兼容性）
