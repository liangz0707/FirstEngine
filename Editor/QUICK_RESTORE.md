# 快速恢复调试配置

## 问题

重新生成项目后，Visual Studio 提示找不到可执行文件：
```
Unable to start program 'G:\AIHUMAN\WorkSpace\FirstEngine\build\x64\Debug\FirstEngineEditor'.
系统找不到指定的文件。
```

## 原因

`.vcxproj.user` 文件在重新生成时被清空，导致调试路径配置丢失。

## 快速解决方案

### 方法 1：运行恢复脚本（推荐）

在 PowerShell 中运行：

```powershell
cd Editor
.\restore_debug_config.ps1
```

### 方法 2：手动恢复

如果脚本不工作，手动创建文件：

1. **打开文件**：`build\Editor\FirstEngineEditor.vcxproj.user`

2. **复制以下内容**：

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
```

3. **保存文件**

4. **重新打开 Visual Studio**

## 验证

1. 关闭并重新打开 Visual Studio
2. 设置 `FirstEngineEditor` 为启动项目
3. 按 F5 测试调试

## 预防措施

### 每次重新生成后

运行恢复脚本：
```powershell
cd Editor
.\restore_debug_config.ps1
```

### 或者添加到构建后步骤

可以在 CMake 中添加后处理步骤，自动恢复配置（需要修改 `Editor/CMakeLists.txt`）。

## 当前状态

✅ 我已经恢复了 `build/Editor/FirstEngineEditor.vcxproj.user` 文件

现在可以：
1. 关闭并重新打开 Visual Studio
2. 测试调试（F5）
