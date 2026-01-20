# 手动设置 Visual Studio 调试路径 - 详细步骤

## 问题
Visual Studio 找不到 `FirstEngineEditor.exe`，因为 CMake 生成的 Utility 项目不知道正确的输出路径。

## 解决方案：在 Visual Studio 中手动设置

### 方法 1：通过项目属性设置（推荐）

1. **打开 Visual Studio**
   - 打开 `build/FirstEngine.sln`

2. **找到 FirstEngineEditor 项目**
   - 在解决方案资源管理器中，找到 `FirstEngineEditor` 项目
   - 注意：它可能在 "Editor" 文件夹下

3. **打开项目属性**
   - 右键点击 `FirstEngineEditor` 项目
   - 选择 **"属性"** (Properties)

4. **查找调试设置**
   - 在左侧树形菜单中，查找以下选项之一：
     - **"调试"** (Debug)
     - **"启动"** (Startup)
     - **"启动选项"** (Startup Options)
   
   如果找不到，尝试：
   - 点击 **"应用程序"** (Application) 标签
   - 查看是否有 **"调试"** 或 **"启动"** 相关的设置

5. **设置启动操作**
   - **启动操作** (Start Action) 或 **启动** (Start):
     - 选择 **"可执行文件"** (Executable) 或 **"外部程序"** (External Program)
   
   - **可执行文件路径** (Executable Path) 或 **程序** (Program):
     - 点击 **"浏览"** (Browse) 按钮
     - 导航到：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\`
     - 选择 `FirstEngineEditor.exe`
     - 或者直接输入：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`

   - **工作目录** (Working Directory):
     - 输入：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows`
     - 或者点击浏览选择该目录

6. **启用本机代码调试**
   - 查找 **"启用本机代码调试"** (Enable native code debugging) 或 **"启用非托管代码调试"** (Enable unmanaged code debugging)
   - ✅ **勾选** 此选项（这对于调试 C++ DLL 很重要）

7. **保存设置**
   - 点击 **"确定"** (OK) 或 **"应用"** (Apply)
   - 关闭属性窗口

8. **测试调试**
   - 在 `App.xaml.cs` 的第 7 行设置断点
   - 按 **F5** 启动调试
   - 如果程序在断点处停止，说明配置成功！

### 方法 2：通过调试配置设置

如果方法 1 找不到设置，尝试：

1. **打开调试配置**
   - 在 Visual Studio 顶部菜单栏
   - 点击 **"调试"** (Debug) → **"调试设置"** (Debug Settings)
   - 或者点击 **"项目"** (Project) → **"FirstEngineEditor 属性"** (FirstEngineEditor Properties)

2. **查找启动设置**
   - 在打开的窗口中查找启动相关的设置
   - 设置可执行文件路径和工作目录

### 方法 3：直接编辑 .vcxproj.user 文件

如果 Visual Studio 没有读取 `.csproj.user` 文件，可以尝试创建 `.vcxproj.user` 文件：

1. **找到 .vcxproj 文件位置**
   - 文件应该在：`build\Editor\FirstEngineEditor.vcxproj`

2. **创建 .vcxproj.user 文件**
   - 在同一目录创建：`build\Editor\FirstEngineEditor.vcxproj.user`
   - 内容如下：

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

3. **重新打开 Visual Studio**

## 验证设置

### 检查文件是否存在

在 PowerShell 中运行：
```powershell
Test-Path "G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
```

应该返回 `True`。

### 检查 Visual Studio 配置

1. 在 Visual Studio 中，查看顶部工具栏
2. 确认配置是 **"Debug"**，平台是 **"x64"**
3. 确认启动项目是 **"FirstEngineEditor"**

### 测试调试

1. 在 `App.xaml.cs` 的 `OnStartup` 方法中设置断点
2. 按 **F5**
3. 如果程序启动并在断点处停止，说明配置成功

## 常见问题

### Q: 找不到"调试"或"启动"标签页

A: 这可能是因为项目类型是 Utility。尝试：
- 右键项目 → 属性 → 查找所有标签页
- 或者使用方法 3 直接编辑 `.vcxproj.user` 文件

### Q: 设置后仍然找不到文件

A: 检查：
1. 文件路径是否正确（注意大小写和反斜杠）
2. 文件是否真的存在
3. Visual Studio 是否已重新加载项目

### Q: 可以启动但无法调试 C++ 代码

A: 确保：
1. **启用本机代码调试** 已勾选
2. C++ DLL 的符号文件 (.pdb) 存在
3. Visual Studio 已安装 C++ 工作负载

## 快速检查清单

- [ ] 文件存在：`Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
- [ ] Visual Studio 配置：Debug x64
- [ ] 启动项目设置为：FirstEngineEditor
- [ ] 可执行文件路径已设置
- [ ] 工作目录已设置
- [ ] 启用本机代码调试已勾选
- [ ] 已重新打开 Visual Studio（如果修改了配置文件）
