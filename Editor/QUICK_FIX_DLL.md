# 快速修复 DLL 加载问题

## 问题

```
Unable to load DLL 'FirstEngine_Core.dll' or one of its dependencies: 找不到指定的模块。 (0x8007007E)
```

## 快速修复（手动）

如果 CMake 自动复制不工作，可以手动复制 DLL：

### 步骤 1：复制 DLL 到编辑器输出目录

在 PowerShell 中执行：

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine

# 设置路径
$sourceDir = "build\bin\Debug"
$destDir = "Editor\bin\Debug\net8.0-windows"

# 确保目标目录存在
New-Item -ItemType Directory -Force -Path $destDir | Out-Null

# 复制 DLL（带 'd' 后缀的 Debug 版本）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Cored.dll" -Force
Copy-Item "$sourceDir\FirstEngine_RHId.dll" "$destDir\FirstEngine_RHId.dll" -Force
Copy-Item "$sourceDir\FirstEngine_Shaderd.dll" "$destDir\FirstEngine_Shaderd.dll" -Force

# 创建不带 'd' 后缀的副本（供 C# P/Invoke 使用）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Core.dll" -Force
Copy-Item "$sourceDir\FirstEngine_RHId.dll" "$destDir\FirstEngine_RHI.dll" -Force
Copy-Item "$sourceDir\FirstEngine_Shaderd.dll" "$destDir\FirstEngine_Shader.dll" -Force

# 复制 vulkan-1.dll（如果存在）
if (Test-Path "$sourceDir\vulkan-1.dll") {
    Copy-Item "$sourceDir\vulkan-1.dll" "$destDir\vulkan-1.dll" -Force
}

Write-Host "DLLs copied successfully!"
Write-Host "Source: $sourceDir"
Write-Host "Destination: $destDir"
```

### 步骤 2：验证文件

```powershell
cd Editor\bin\Debug\net8.0-windows
dir *.dll
```

应该看到：
- `FirstEngine_Cored.dll` 和 `FirstEngine_Core.dll`
- `FirstEngine_RHId.dll` 和 `FirstEngine_RHI.dll`
- `FirstEngine_Shaderd.dll` 和 `FirstEngine_Shader.dll`
- `vulkan-1.dll`

### 步骤 3：运行编辑器

现在应该可以正常加载 DLL 了。

## 自动修复（通过 CMake）

重新生成 CMake 配置并编译：

```powershell
cd build
cmake --build . --config Debug --target FirstEngineEditor
```

CMake 应该会自动复制 DLL 并创建不带 'd' 后缀的副本。
