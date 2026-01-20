# 快速修复 DLL 导出问题

## 问题

- 包含 `FE_CORE_API` → 编译错误
- 不包含 `FE_CORE_API` → 函数不被导出

## 解决方案：使用 EDITOR_API_EXPORT

已在 `EditorAPI.cpp` 中实现：

### 1. 定义导出宏

在 `extern "C"` 块之前：

```cpp
// Define export macro for EditorAPI functions
// Since EditorAPI.cpp is compiled into FirstEngine_Core.dll, we always export
#ifdef _WIN32
    #define EDITOR_API_EXPORT __declspec(dllexport)
#else
    #define EDITOR_API_EXPORT
#endif
```

### 2. 在所有函数定义中使用

```cpp
extern "C" {
    EDITOR_API_EXPORT void* EditorAPI_CreateEngine(...) { ... }
    EDITOR_API_EXPORT void EditorAPI_DestroyEngine(...) { ... }
    // ...
}
```

## 工作原理

- `EDITOR_API_EXPORT` 直接展开为 `__declspec(dllexport)`
- 不依赖 `FirstEngine_Core_EXPORTS` 宏
- 不依赖 `FE_CORE_API` 宏
- 因为 `EditorAPI.cpp` 是 `FirstEngine_Core.dll` 的一部分，所以总是导出

## 现在请执行

### 步骤 1：重新编译

```powershell
cd build
cmake --build . --config Debug
```

### 步骤 2：验证导出

```powershell
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
& $dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String -Pattern "EditorAPI_CreateEngine"
```

### 步骤 3：复制 DLL

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine
Copy-Item "build\bin\Debug\FirstEngine_Cored.dll" "Editor\bin\Debug\net8.0-windows\FirstEngine_Cored.dll" -Force
Copy-Item "build\bin\Debug\FirstEngine_Cored.dll" "Editor\bin\Debug\net8.0-windows\FirstEngine_Core.dll" -Force
```

## 优势

1. **简单直接**：不依赖复杂的宏系统
2. **可靠**：直接使用 `__declspec(dllexport)`
3. **明确**：清楚地表明这些函数需要导出
4. **无副作用**：不影响头文件或其他代码

## 备选方案：.def 文件

如果 `EDITOR_API_EXPORT` 方法不工作，可以使用 `.def` 文件（已创建 `src/Editor/EditorAPI.def`）。

查看 `FIX_DLL_EXPORT_DEF.md` 了解详细信息。
