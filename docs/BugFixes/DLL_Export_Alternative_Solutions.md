# DLL 函数导出问题的替代解决方案

## 问题

- **包含 `FE_CORE_API`**：编译错误 `definition of dllimport function not allowed`
- **不包含 `FE_CORE_API`**：函数不被导出，运行时错误 `Unable to find an entry point`

## 解决方案 1：使用 .def 文件（推荐）

### 优点
- 不依赖宏定义
- 显式控制导出函数
- 最可靠的方法

### 实现

1. **创建 `.def` 文件**：`src/Editor/EditorAPI.def`
2. **在 CMakeLists.txt 中链接**：
   ```cmake
   set_target_properties(FirstEngine_Core PROPERTIES
       LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def"
   )
   ```
3. **实现文件**：不需要 `FE_CORE_API`

## 解决方案 2：在实现文件中直接使用 __declspec(dllexport)

### 优点
- 不需要 `.def` 文件
- 不依赖宏

### 实现

在 `EditorAPI.cpp` 开头添加：

```cpp
#ifdef _WIN32
    // Always export when compiling this file (it's part of FirstEngine_Core DLL)
    #define EDITOR_API_EXPORT __declspec(dllexport)
#else
    #define EDITOR_API_EXPORT
#endif
```

然后在所有函数定义中使用：

```cpp
extern "C" {
    EDITOR_API_EXPORT void* EditorAPI_CreateEngine(...) { ... }
    EDITOR_API_EXPORT void EditorAPI_DestroyEngine(...) { ... }
    // ...
}
```

## 解决方案 3：修复宏展开问题

### 问题
`FirstEngine_Core_EXPORTS` 可能在编译 `EditorAPI.cpp` 时没有被定义。

### 检查方法

在 `EditorAPI.cpp` 开头添加：

```cpp
#ifdef FirstEngine_Core_EXPORTS
    #pragma message("FirstEngine_Core_EXPORTS is defined")
#else
    #pragma message("WARNING: FirstEngine_Core_EXPORTS is NOT defined")
#endif
```

### 修复

确保 CMake 在编译 `EditorAPI.cpp` 时定义宏：

```cmake
# 方法 1：使用 target_compile_definitions（推荐）
target_compile_definitions(FirstEngine_Core PRIVATE FirstEngine_Core_EXPORTS)

# 方法 2：在源文件级别定义
set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.cpp
    PROPERTIES COMPILE_DEFINITIONS "FirstEngine_Core_EXPORTS"
)
```

## 解决方案 4：使用条件编译

在 `EditorAPI.cpp` 中：

```cpp
// 在文件开头定义导出宏
#ifdef _WIN32
    #ifdef FirstEngine_Core_EXPORTS
        #define EDITOR_API_IMPL __declspec(dllexport)
    #else
        #define EDITOR_API_IMPL __declspec(dllimport)
    #endif
#else
    #define EDITOR_API_IMPL
#endif

extern "C" {
    // 使用 EDITOR_API_IMPL 而不是 FE_CORE_API
    EDITOR_API_IMPL void* EditorAPI_CreateEngine(...) { ... }
}
```

但这种方法仍然需要 `FirstEngine_Core_EXPORTS` 被定义。

## 推荐方案

**使用 `.def` 文件（解决方案 1）**，因为：
1. 最可靠，不依赖宏
2. 显式控制导出
3. 适合 C 风格函数（`extern "C"`）
4. 避免宏展开问题

## 快速实施

### 步骤 1：使用 .def 文件（已创建）

`.def` 文件已创建：`src/Editor/EditorAPI.def`

### 步骤 2：更新 CMakeLists.txt（已更新）

已添加链接 `.def` 文件的配置。

### 步骤 3：重新生成和编译

```powershell
cd build
cmake ..
cmake --build . --config Debug
```

### 步骤 4：验证

```powershell
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
& $dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String -Pattern "EditorAPI_CreateEngine"
```

应该看到函数被导出。
