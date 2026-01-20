# 编辑器启动问题修复

## 问题描述

运行主程序时提示：
```
Error: Could not find FirstEngineEditor.exe
Please ensure the editor is built and available in the same directory or parent directory.
```

## 修复内容

### 1. 编译编辑器

首先确保编辑器已编译：

```powershell
cd Editor
dotnet build FirstEngineEditor.csproj -c Debug
```

编辑器可执行文件位置：
- `Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
- `Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe`

### 2. 修改查找逻辑

已更新 `src/Application/RenderApp.cpp` 中的编辑器查找逻辑，现在会在以下位置查找：

1. **同目录**：`FirstEngine.exe` 所在目录
2. **父目录**：`FirstEngine.exe` 的父目录
3. **Editor Debug**：`../../Editor/bin/Debug/net8.0-windows/FirstEngineEditor.exe`
4. **Editor Release**：`../../Editor/bin/Release/net8.0-windows/FirstEngineEditor.exe`
5. **Editor Debug (从 build/bin)**：`../Editor/bin/Debug/net8.0-windows/FirstEngineEditor.exe`
6. **Editor Release (从 build/bin)**：`../Editor/bin/Release/net8.0-windows/FirstEngineEditor.exe`

### 3. 路径规范化

代码会自动：
- 将路径中的 `/` 转换为 `\`
- 解析 `..` 相对路径
- 使用 `GetFileAttributesA` 检查文件是否存在

### 4. 错误信息改进

如果找不到编辑器，会显示：
- 所有搜索过的路径
- 如何编译编辑器的提示

## 使用方法

### 方法 1：直接运行编辑器

```powershell
cd Editor\bin\Debug\net8.0-windows
.\FirstEngineEditor.exe
```

### 方法 2：通过主程序启动（使用 --editor 参数）

```powershell
# 从 build\bin\Debug 或 build\bin\Release 运行
cd build\bin\Debug
.\FirstEngine.exe --editor
```

主程序会自动查找并启动编辑器。

### 方法 3：将编辑器复制到主程序目录

```powershell
# 复制编辑器到主程序输出目录
Copy-Item "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe" "build\bin\Debug\"
```

## 验证

运行以下命令验证编辑器是否可找到：

```powershell
# 检查编辑器是否存在
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"

# 检查主程序是否存在
Test-Path "build\bin\Debug\FirstEngine.exe"
```

## 注意事项

1. **编辑器必须先编译**：在运行 `--editor` 模式之前，确保编辑器已编译
2. **路径关系**：编辑器查找基于 `FirstEngine.exe` 的位置，确保路径关系正确
3. **Debug vs Release**：编辑器会优先查找 Debug 版本，如果找不到再查找 Release 版本

## 编译顺序

推荐的完整构建流程：

```powershell
# 1. 构建 C++ 主工程
.\build_project.ps1

# 2. 构建 C# 编辑器
cd Editor
dotnet build FirstEngineEditor.csproj -c Debug
cd ..

# 3. 运行（编辑器模式）
cd build\bin\Debug
.\FirstEngine.exe --editor
```
