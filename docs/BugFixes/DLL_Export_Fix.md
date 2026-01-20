# DLL 函数导出问题修复

## 问题描述

在 C# 编辑器调用 C++ DLL 时，出现以下错误：

```
Unable to find an entry point named 'EditorAPI_CreateEngine' in DLL 'FirstEngine_Core.dll'.
```

或者编译时出现：

```
'EditorAPI_CreateEngine': definition of dllimport function not allowed
```

## 问题原因

在 Windows DLL 中，`extern "C"` 函数的导出机制存在以下问题：

1. **头文件声明** (`EditorAPI.h`) 中使用 `FE_CORE_API` 宏：
   - 编译 DLL 时（定义了 `FirstEngine_Core_EXPORTS`），`FE_CORE_API` 展开为 `__declspec(dllexport)`
   - 使用 DLL 时（未定义 `FirstEngine_Core_EXPORTS`），`FE_CORE_API` 展开为 `__declspec(dllimport)`

2. **实现文件定义** (`EditorAPI.cpp`) 中：
   - 如果使用 `FE_CORE_API`，但编译时 `FirstEngine_Core_EXPORTS` 未定义，会导致 `dllimport` 错误
   - 如果不使用 `FE_CORE_API`，函数不会被导出，导致找不到入口点

## 最终解决方案

### 方案 1：使用 EDITOR_API_EXPORT 宏（推荐）

在 `EditorAPI.cpp` 中定义独立的导出宏：

```cpp
// Define export macro for EditorAPI functions
// Since EditorAPI.cpp is compiled into FirstEngine_Core.dll, we always export
#ifdef _WIN32
    #define EDITOR_API_EXPORT __declspec(dllexport)
#else
    #define EDITOR_API_EXPORT
#endif

extern "C" {
    EDITOR_API_EXPORT void* EditorAPI_CreateEngine(...) { ... }
    EDITOR_API_EXPORT void EditorAPI_DestroyEngine(...) { ... }
    // ... 其他函数
}
```

**优点**：
- 简单直接，不依赖复杂的宏系统
- 明确表明这些函数需要导出
- 不依赖 `FirstEngine_Core_EXPORTS` 定义

### 方案 2：使用 .def 文件（备选）

创建 `src/Editor/EditorAPI.def` 文件：

```def
EXPORTS
EditorAPI_CreateEngine
EditorAPI_DestroyEngine
EditorAPI_InitializeEngine
EditorAPI_ShutdownEngine
EditorAPI_CreateViewport
EditorAPI_DestroyViewport
EditorAPI_ResizeViewport
EditorAPI_SetViewportActive
EditorAPI_GetViewportWindowHandle
EditorAPI_PrepareFrameGraph
EditorAPI_CreateResources
EditorAPI_Render
EditorAPI_SubmitFrame
EditorAPI_BeginFrame
EditorAPI_EndFrame
EditorAPI_RenderViewport
EditorAPI_IsViewportReady
EditorAPI_LoadScene
EditorAPI_UnloadScene
EditorAPI_LoadResource
EditorAPI_UnloadResource
EditorAPI_SetRenderConfig
```

在 `src/Core/CMakeLists.txt` 中链接：

```cmake
if(WIN32)
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def")
        set_target_properties(FirstEngine_Core PROPERTIES
            LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def"
        )
    endif()
endif()
```

**优点**：
- 最可靠的导出方式
- 不依赖编译器宏
- 可以精确控制导出的函数

## 验证步骤

### 1. 重新编译项目

```powershell
cd build
cmake --build . --config Debug
```

### 2. 验证函数导出

使用 `dumpbin` 检查导出的函数：

```powershell
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
& $dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String -Pattern "EditorAPI_CreateEngine"
```

应该看到所有 `EditorAPI_*` 函数。

### 3. 复制 DLL 到编辑器目录

```powershell
$sourceDir = "build\bin\Debug"
$destDir = "Editor\bin\Debug\net8.0-windows"

# 复制 DLL（带 'd' 后缀）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Cored.dll" -Force

# 创建不带 'd' 后缀的副本（供 C# 使用）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Core.dll" -Force
```

### 4. 运行编辑器测试

现在应该可以正常加载 DLL 并找到函数入口点了。

## 相关文档

- [DLL导出问题修复-最终方案](./DLL_Export_Fix_Final.md) - 历史方案（已过时）
- [DLL导出问题快速修复](./DLL_Export_Quick_Fix.md) - 快速修复指南
- [DLL导出问题修复-使用DEF文件](./DLL_Export_Fix_Using_DEF.md) - DEF文件方案详情
- [DLL导出替代方案](./DLL_Export_Alternative_Solutions.md) - 所有方案对比

## 注意事项

1. **函数名称修饰**：确保函数在 `extern "C"` 块中，避免C++名称修饰
2. **调用约定**：C# 中的 `CallingConvention` 应与 C++ 匹配（默认 `Cdecl`）
3. **DLL名称**：Debug 配置下 DLL 有 'd' 后缀，需要创建不带后缀的副本供 C# 使用
