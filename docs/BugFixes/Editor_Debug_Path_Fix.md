# 修复 Visual Studio 调试路径问题 - 快速指南

## 问题

Visual Studio 在错误路径查找可执行文件：
- ❌ 查找：`build\x64\Debug\FirstEngineEditor`
- ✅ 实际：`Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`

## 已完成的修复

我已经更新了 `FirstEngineEditor.csproj.user` 文件，配置了正确的路径。

## 现在请按以下步骤操作

### 步骤 1：关闭并重新打开 Visual Studio

1. 完全关闭 Visual Studio
2. 重新打开 `build/FirstEngine.sln`

### 步骤 2：设置启动项目

1. 在解决方案资源管理器中
2. 右键 `FirstEngineEditor` 项目
3. 选择"设为启动项目"

### 步骤 3：测试调试

1. 在 `App.xaml.cs` 的第 7 行设置断点
2. 按 F5 启动调试
3. 如果程序在断点处停止，说明配置成功！

## 如果仍然提示"找不到文件"

### 方案 A：使用绝对路径（备用）

如果 `$(SolutionDir)` 变量不工作，可以使用绝对路径：

1. 备份当前的 `.csproj.user` 文件
2. 将 `FirstEngineEditor.csproj.user.absolute` 重命名为 `FirstEngineEditor.csproj.user`
3. 重新打开 Visual Studio 测试

### 方案 B：在 Visual Studio 中手动设置

1. 右键 `FirstEngineEditor` 项目 → 属性
2. 查找"调试"或"启动"标签页
3. 设置：
   - **启动操作**：可执行文件
   - **可执行文件**：浏览选择 `Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
   - **工作目录**：`Editor\bin\Debug\net8.0-windows`
   - **启用本机代码调试**：✅ 勾选

### 方案 C：检查项目属性

1. 右键项目 → 属性
2. 查看"生成"标签
3. 确认"输出路径"是：`bin\Debug\net8.0-windows\`

## 验证文件存在

运行以下命令确认文件存在：

```powershell
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
```

应该返回 `True`。

## 当前配置说明

`.csproj.user` 文件现在使用：
- `StartAction`: `Program`（直接启动可执行文件）
- `StartProgram`: `$(SolutionDir)Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
- `StartWorkingDirectory`: `$(SolutionDir)Editor\bin\Debug\net8.0-windows`
- `EnableUnmanagedDebugging`: `true`（启用 C++ 调试）

`$(SolutionDir)` 变量会解析为解决方案文件所在的目录（`build/`），所以完整路径应该是：
`G:\AIHUMAN\WorkSpace\FirstEngine\build\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`

但实际文件在：
`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`

所以路径需要从 `build/` 回到项目根目录。让我修正路径：