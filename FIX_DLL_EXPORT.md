# 修复 DLL 函数导出问题

## 问题

```
Unable to find an entry point named 'EditorAPI_CreateEngine' in DLL 'FirstEngine_Core.dll'.
```

## 原因

`EditorAPI.cpp` 中的函数定义缺少 `FE_CORE_API` 宏，导致函数没有被正确导出。

虽然头文件 `EditorAPI.h` 中声明了 `FE_CORE_API`，但实现文件中的函数定义没有使用这个宏，所以函数没有被标记为 `__declspec(dllexport)`。

## 已完成的修复

我已经为所有 EditorAPI 函数添加了 `FE_CORE_API` 宏：

- `EditorAPI_CreateEngine`
- `EditorAPI_DestroyEngine`
- `EditorAPI_InitializeEngine`
- `EditorAPI_ShutdownEngine`
- `EditorAPI_CreateViewport`
- `EditorAPI_DestroyViewport`
- `EditorAPI_ResizeViewport`
- `EditorAPI_SetViewportActive`
- `EditorAPI_GetViewportWindowHandle`
- `EditorAPI_PrepareFrameGraph`
- `EditorAPI_CreateResources`
- `EditorAPI_Render`
- `EditorAPI_SubmitFrame`
- `EditorAPI_BeginFrame`
- `EditorAPI_EndFrame`
- `EditorAPI_RenderViewport`
- `EditorAPI_IsViewportReady`
- `EditorAPI_LoadScene`
- `EditorAPI_UnloadScene`
- `EditorAPI_LoadResource`
- `EditorAPI_UnloadResource`
- `EditorAPI_SetRenderConfig`

## 现在请执行

### 步骤 1：重新编译 C++ 项目

在 Visual Studio 中：
1. 右键解决方案 → **"重新生成解决方案"** (Rebuild Solution)

或在命令行：
```powershell
cd build
cmake --build . --config Debug
```

### 步骤 2：验证函数导出

使用 `dumpbin` 检查导出的函数：

```powershell
# 需要 Visual Studio Developer Command Prompt
dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String "EditorAPI"
```

应该看到所有 `EditorAPI_*` 函数。

### 步骤 3：重新复制 DLL

如果 DLL 已经复制到编辑器输出目录，需要重新复制：

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine
$sourceDir = "build\bin\Debug"
$destDir = "Editor\bin\Debug\net8.0-windows"

# 复制 DLL（带 'd' 后缀）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Cored.dll" -Force

# 创建不带 'd' 后缀的副本（供 C# 使用）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Core.dll" -Force

Write-Host "DLL copied successfully!"
```

### 步骤 4：运行编辑器

现在应该可以正常加载 DLL 并找到函数入口点了。

## 验证清单

- [ ] 重新编译了 C++ 项目
- [ ] 验证了函数导出（使用 dumpbin）
- [ ] 重新复制了 DLL 到编辑器输出目录
- [ ] 确认有 `FirstEngine_Core.dll`（不带 'd' 后缀）
- [ ] 尝试运行编辑器
- [ ] 不再出现 "Unable to find an entry point" 错误

## 如果仍然有问题

### 方法 1：检查函数名称修饰

C# P/Invoke 期望的是 C 风格的函数名（无名称修饰）。确保：

1. 函数在 `extern "C"` 块中
2. 函数使用了 `FE_CORE_API` 宏
3. 函数名称与 C# 中的 `DllImport` 声明完全匹配

### 方法 2：使用 .def 文件

如果仍然有问题，可以创建一个 `.def` 文件来显式导出函数：

```def
EXPORTS
EditorAPI_CreateEngine
EditorAPI_DestroyEngine
EditorAPI_InitializeEngine
...
```

然后在 CMakeLists.txt 中：

```cmake
set_target_properties(FirstEngine_Core PROPERTIES
    LINK_FLAGS "/DEF:${CMAKE_CURRENT_SOURCE_DIR}/EditorAPI.def"
)
```

### 方法 3：检查调用约定

确保 C# 中的 `CallingConvention` 与 C++ 中的匹配：

```csharp
[DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
```

C++ 中的 `extern "C"` 函数默认使用 C 调用约定（`__cdecl`）。
