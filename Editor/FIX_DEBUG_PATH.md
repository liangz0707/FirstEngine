# 修复 Visual Studio 调试路径问题

## 问题描述

Visual Studio 在错误的路径查找可执行文件：
- **Visual Studio 查找**：`G:\AIHUMAN\WorkSpace\FirstEngine\build\x64\Debug\FirstEngineEditor`
- **实际文件位置**：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`

## 原因

CMake 生成的 `FirstEngineEditor.vcxproj` 是一个 **Utility** 类型的项目（不是可执行文件项目），所以 Visual Studio 不知道正确的输出路径。

## 解决方案

我已经更新了 `FirstEngineEditor.csproj.user` 文件，使用绝对路径指向正确的可执行文件位置。

### 方法 1：使用 .csproj.user 文件（已配置）

`.csproj.user` 文件已经配置好了，使用 `$(SolutionDir)` 变量指向正确的路径：
- Debug: `$(SolutionDir)Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
- Release: `$(SolutionDir)Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe`

**操作步骤**：
1. 关闭 Visual Studio（如果已打开）
2. 重新打开 `build/FirstEngine.sln`
3. 右键 `FirstEngineEditor` 项目 → 设为启动项目
4. 按 F5 启动调试

### 方法 2：在 Visual Studio 中手动设置

如果 `.csproj.user` 文件不生效，可以手动设置：

1. **右键 `FirstEngineEditor` 项目 → 属性**
2. **查找"调试"或"启动"标签页**
3. **设置以下内容**：
   - **启动操作**：选择"可执行文件"或"外部程序"
   - **可执行文件**：浏览并选择 `Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
   - **工作目录**：`Editor\bin\Debug\net8.0-windows`
   - **启用本机代码调试**：✅ 勾选

### 方法 3：使用 launchSettings.json

如果上述方法都不行，创建 `Properties/launchSettings.json`：

1. 在 `Editor` 文件夹下创建 `Properties` 文件夹
2. 创建 `launchSettings.json`：

```json
{
  "profiles": {
    "FirstEngineEditor": {
      "commandName": "Executable",
      "executablePath": "$(SolutionDir)Editor\\bin\\Debug\\net8.0-windows\\FirstEngineEditor.exe",
      "workingDirectory": "$(SolutionDir)Editor\\bin\\Debug\\net8.0-windows",
      "nativeDebugging": true
    }
  }
}
```

## 验证修复

1. **检查文件是否存在**：
   ```powershell
   Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
   ```
   应该返回 `True`

2. **在 Visual Studio 中测试**：
   - 设置断点（例如在 `App.xaml.cs` 的 `OnStartup` 方法）
   - 按 F5
   - 如果程序在断点处停止，说明路径配置正确

3. **检查输出窗口**：
   - 如果仍然失败，查看 Visual Studio 的输出窗口
   - 查找具体的错误消息

## 如果仍然无法工作

### 检查解决方案目录

确保 `$(SolutionDir)` 变量解析正确：
- 解决方案文件应该在 `build/FirstEngine.sln`
- `$(SolutionDir)` 应该解析为 `G:\AIHUMAN\WorkSpace\FirstEngine\build\`

### 使用绝对路径（临时方案）

如果相对路径不工作，可以临时使用绝对路径：

```xml
<StartProgram>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe</StartProgram>
<StartWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows</StartWorkingDirectory>
```

### 检查项目类型

在 Visual Studio 中：
1. 右键 `FirstEngineEditor` 项目 → 属性
2. 查看"应用程序"标签
3. 确认"输出类型"是"Windows 应用程序"

## 常见问题

### Q: 为什么 CMake 生成的是 Utility 项目？

A: 因为 C# 项目不是 CMake 直接管理的，CMake 只是通过 `add_custom_target` 添加了一个构建目标，所以类型是 Utility。

### Q: 能否让 CMake 生成正确的项目类型？

A: 可以，但需要更复杂的配置。使用 `.csproj.user` 文件是更简单的解决方案。

### Q: 每次重新生成 CMake 配置会覆盖设置吗？

A: 不会。`.csproj.user` 文件不会被 CMake 覆盖，它是用户特定的配置文件。

## 快速修复清单

- [x] 更新 `.csproj.user` 文件使用正确的路径
- [ ] 关闭并重新打开 Visual Studio
- [ ] 设置 `FirstEngineEditor` 为启动项目
- [ ] 验证可执行文件存在
- [ ] 测试调试（F5）
