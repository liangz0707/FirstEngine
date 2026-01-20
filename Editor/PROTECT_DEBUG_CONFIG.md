# 保护调试配置不被清理

## 问题

重新生成项目后，`.vcxproj.user` 文件被清空，导致调试路径配置丢失。

## 解决方案

### 方案 1：将配置添加到 .gitignore 排除列表（推荐）

`.vcxproj.user` 文件通常被 `.gitignore` 忽略，但我们可以创建一个模板文件：

1. **创建模板文件**：`build/Editor/FirstEngineEditor.vcxproj.user.template`
2. **在 CMake 后处理脚本中复制模板**

### 方案 2：在 CMake 中自动生成（最佳）

修改 `Editor/CMakeLists.txt`，在生成项目后自动创建 `.vcxproj.user` 文件。

### 方案 3：手动恢复（临时方案）

每次重新生成后，手动恢复 `.vcxproj.user` 文件。

## 当前状态

我已经恢复了 `build/Editor/FirstEngineEditor.vcxproj.user` 文件。

## 防止再次丢失的方法

### 方法 1：创建备份脚本

创建一个 PowerShell 脚本来恢复配置：

```powershell
# restore_debug_config.ps1
$config = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Release|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
"@

$path = "build\Editor\FirstEngineEditor.vcxproj.user"
$config | Out-File -FilePath $path -Encoding UTF8
Write-Host "Debug configuration restored: $path"
```

### 方法 2：在 Visual Studio 中设置（永久）

在 Visual Studio 项目属性中手动设置，这样即使 `.vcxproj.user` 被删除，设置也会保存在项目文件中。

## 快速恢复命令

如果配置再次丢失，运行：

```powershell
cd build\Editor
@"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Debug|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Release|x64'">
    <LocalDebuggerCommand>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe</LocalDebuggerCommand>
    <LocalDebuggerWorkingDirectory>G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Release\net8.0-windows</LocalDebuggerWorkingDirectory>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
"@ | Out-File -FilePath "FirstEngineEditor.vcxproj.user" -Encoding UTF8
```

## 注意事项

- `.vcxproj.user` 文件是用户特定的，不会被 CMake 管理
- 重新生成 CMake 项目可能会删除此文件
- 建议将配置保存在版本控制之外，但创建恢复脚本
