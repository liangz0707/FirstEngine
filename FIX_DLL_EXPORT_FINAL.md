# 修复 DLL 函数导出问题（最终方案）

## 问题

编译错误：`'EditorAPI_CreateEngine': definition of dllimport function not allowed`

## 原因

在 Windows DLL 中，`extern "C"` 函数的导出机制：

1. **头文件** (`EditorAPI.h`) 中声明：`FE_CORE_API void* EditorAPI_CreateEngine(...);`
   - 当编译 DLL 时（定义了 `FirstEngine_Core_EXPORTS`），`FE_CORE_API` 展开为 `__declspec(dllexport)`
   - 当使用 DLL 时（未定义 `FirstEngine_Core_EXPORTS`），`FE_CORE_API` 展开为 `__declspec(dllimport)`

2. **实现文件** (`EditorAPI.cpp`) 中定义：`void* EditorAPI_CreateEngine(...) { ... }`
   - **不应该**包含 `FE_CORE_API`
   - 只需要匹配头文件的函数签名（不包括导出宏）

3. **问题**：如果实现文件中使用了 `FE_CORE_API`，而编译时 `FirstEngine_Core_EXPORTS` 没有被定义，那么 `FE_CORE_API` 会被展开为 `__declspec(dllimport)`，导致编译错误。

## 已完成的修复

### 1. 移除实现文件中的 `FE_CORE_API`

所有 EditorAPI 函数的实现中，移除了 `FE_CORE_API` 宏：
- `void* EditorAPI_CreateEngine(...)`（不是 `FE_CORE_API void* EditorAPI_CreateEngine(...)`）
- 其他所有函数同样处理

### 2. 确保 CMake 正确定义 `FirstEngine_Core_EXPORTS`

在 `src/Core/CMakeLists.txt` 中：
```cmake
# Set export macro for this module (must be after add_library)
if(WIN32)
    target_compile_definitions(FirstEngine_Core PRIVATE FirstEngine_Core_EXPORTS)
endif()
```

这确保编译 `FirstEngine_Core` DLL 时，`FirstEngine_Core_EXPORTS` 被定义，头文件中的 `FE_CORE_API` 会被展开为 `__declspec(dllexport)`。

## 工作原理

1. **编译 DLL 时**（`EditorAPI.cpp` 在 `FirstEngine_Core` 项目中）：
   - `FirstEngine_Core_EXPORTS` 被定义
   - `EditorAPI.h` 中的 `FE_CORE_API` 展开为 `__declspec(dllexport)`
   - 函数被正确导出

2. **使用 DLL 时**（C# 编辑器）：
   - `FirstEngine_Core_EXPORTS` 未被定义
   - `EditorAPI.h` 中的 `FE_CORE_API` 展开为 `__declspec(dllimport)`
   - 函数从 DLL 导入

3. **实现文件**：
   - 不包含 `FE_CORE_API`，只匹配函数签名
   - 函数定义与头文件声明匹配（不包括导出宏）

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

重新编译后，检查 `EditorAPI_CreateEngine` 是否被导出：

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine
$dumpbin = "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64\dumpbin.exe"
& $dumpbin /exports "build\bin\Debug\FirstEngine_Cored.dll" | Select-String -Pattern "EditorAPI_CreateEngine"
```

应该看到 `EditorAPI_CreateEngine` 在导出列表中。

### 步骤 3：重新复制 DLL

重新编译后，重新复制 DLL 到编辑器输出目录：

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

现在应该可以正常加载 DLL 并找到 `EditorAPI_CreateEngine` 函数了。

## 关键点

1. **头文件**：使用 `FE_CORE_API` 声明函数
2. **实现文件**：**不使用** `FE_CORE_API`，只匹配函数签名
3. **CMake**：确保编译 DLL 时定义 `FirstEngine_Core_EXPORTS`

## 如果仍然有问题

检查：
1. `FirstEngine_Core_EXPORTS` 是否在编译 `EditorAPI.cpp` 时被定义
2. 头文件 `EditorAPI.h` 是否被正确包含
3. `FE_CORE_API` 宏的定义是否正确

可以在 `EditorAPI.cpp` 开头添加：
```cpp
#ifdef FirstEngine_Core_EXPORTS
    #pragma message("FirstEngine_Core_EXPORTS is defined")
#else
    #pragma message("FirstEngine_Core_EXPORTS is NOT defined")
#endif
```

编译时应该看到 "FirstEngine_Core_EXPORTS is defined" 消息。
