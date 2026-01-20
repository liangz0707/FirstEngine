# 使用 .def 文件修复 DLL 函数导出问题

## 问题

- 如果实现文件中包含 `FE_CORE_API`，会报错：`definition of dllimport function not allowed`
- 如果不包含 `FE_CORE_API`，函数不会被导出，导致：`Unable to find an entry point named 'EditorAPI_CreateEngine'`

## 解决方案：使用 .def 文件

使用 `.def` 文件显式导出函数是最可靠的方法，不依赖于 `__declspec(dllexport)` 宏。

### 1. 创建 .def 文件

已创建 `src/Editor/EditorAPI.def`，包含所有需要导出的函数：

```
EXPORTS
EditorAPI_CreateEngine
EditorAPI_DestroyEngine
...
```

### 2. 在 CMakeLists.txt 中链接 .def 文件

已在 `src/Core/CMakeLists.txt` 中添加：

```cmake
if(WIN32)
    # Use .def file to explicitly export EditorAPI functions
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def")
        set_target_properties(FirstEngine_Core PROPERTIES
            LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def"
        )
    endif()
endif()
```

### 3. 实现文件不需要 FE_CORE_API

实现文件中的函数定义不需要 `FE_CORE_API`：

```cpp
// 正确：不使用 FE_CORE_API
void* EditorAPI_CreateEngine(void* windowHandle, int width, int height) {
    // ...
}
```

## 工作原理

1. **`.def` 文件**：显式列出所有需要导出的函数
2. **链接器**：使用 `/DEF:` 选项告诉链接器使用 `.def` 文件
3. **函数导出**：链接器根据 `.def` 文件导出函数，不依赖于 `__declspec(dllexport)`

## 优势

1. **不依赖宏**：不需要 `FirstEngine_Core_EXPORTS` 宏
2. **显式控制**：明确列出所有导出的函数
3. **更可靠**：避免宏展开问题
4. **C 风格函数**：特别适合 `extern "C"` 函数

## 现在请执行

### 步骤 1：重新生成 CMake 配置

```powershell
cd build
cmake ..
```

### 步骤 2：重新编译

```powershell
cmake --build . --config Debug
```

### 步骤 3：验证函数导出

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
& $dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String -Pattern "EditorAPI_CreateEngine"
```

应该看到 `EditorAPI_CreateEngine` 在导出列表中。

### 步骤 4：重新复制 DLL

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine
$sourceDir = "build\bin\Debug"
$destDir = "Editor\bin\Debug\net8.0-windows"

Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Cored.dll" -Force
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Core.dll" -Force

Write-Host "DLL copied successfully!"
```

## 验证清单

- [ ] `.def` 文件已创建（`src/Editor/EditorAPI.def`）
- [ ] CMakeLists.txt 已更新（链接 `.def` 文件）
- [ ] 重新生成了 CMake 配置
- [ ] 重新编译了项目
- [ ] 验证了函数导出（使用 dumpbin）
- [ ] 重新复制了 DLL
- [ ] 运行编辑器测试

## 如果仍然有问题

### 检查 .def 文件路径

确保 CMake 能找到 `.def` 文件：

```cmake
message(STATUS "EditorAPI.def path: ${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def")
message(STATUS "EditorAPI.def exists: ${EXISTS "${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.def"}")
```

### 检查链接器选项

在 Visual Studio 中：
1. 右键 `FirstEngine_Core` 项目 → 属性
2. 配置属性 → 链接器 → 输入 → 模块定义文件
3. 应该看到 `EditorAPI.def` 的路径

### 替代方案：直接使用 __declspec(dllexport)

如果 `.def` 文件不工作，可以在实现文件中直接使用：

```cpp
#ifdef _WIN32
    #define EDITOR_API_EXPORT __declspec(dllexport)
#else
    #define EDITOR_API_EXPORT
#endif

extern "C" {
    EDITOR_API_EXPORT void* EditorAPI_CreateEngine(...) { ... }
}
```

但这需要确保只在编译 DLL 时使用 `EDITOR_API_EXPORT`。
