# FirstEngine 编辑器调试设置详细指南

## 问题：找不到调试设置选项

在 Visual Studio 2022 中，调试设置的界面可能有所不同。以下是详细的设置步骤：

## 步骤 1：确保项目已构建

**重要**：在设置调试之前，必须先构建项目！

### 方法 A：使用命令行构建

```powershell
# 1. 构建 C++ 引擎
cd g:\AIHUMAN\WorkSpace\FirstEngine
.\build_project.ps1

# 2. 构建 C# 编辑器
cd Editor
dotnet build -c Debug
```

### 方法 B：在 Visual Studio 中构建

1. 打开 `build/FirstEngine.sln`
2. 选择配置：**Debug** 和 **x64**
3. 生成 → 生成解决方案（或按 Ctrl+Shift+B）
4. 等待构建完成

## 步骤 2：验证可执行文件存在

构建完成后，检查文件是否存在：

```powershell
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
```

如果返回 `False`，说明构建失败，需要先解决构建问题。

## 步骤 3：在 Visual Studio 中设置调试

### 3.1 打开项目

**选项 A：打开解决方案（推荐）**
- 打开 `build/FirstEngine.sln`
- 在解决方案资源管理器中找到 `FirstEngineEditor` 项目

**选项 B：直接打开项目文件**
- 文件 → 打开 → 项目/解决方案
- 选择 `Editor/FirstEngineEditor.csproj`

### 3.2 设置启动项目

1. 在**解决方案资源管理器**中，右键点击 `FirstEngineEditor` 项目
2. 选择**"设为启动项目"**（Set as Startup Project）

### 3.3 配置调试设置（Visual Studio 2022）

在 Visual Studio 2022 中，调试设置的路径可能不同：

#### 方法 1：通过项目属性（推荐）

1. 右键点击 `FirstEngineEditor` 项目
2. 选择**"属性"**（Properties）
3. 在左侧选择**"调试"**（Debug）或**"调试"**（Debugging）
4. 如果看不到"调试"选项，尝试：
   - 点击左侧的**"调试"**标签
   - 或者查看**"调试"**（Debug）部分

5. 在调试页面中，找到以下设置：
   - **启动操作**（Launch）：选择"启动项目"或"可执行文件"
   - **启用本机代码调试**（Enable native code debugging）：✅ **勾选此选项**（非常重要！）
   - **工作目录**（Working directory）：`$(OutDir)` 或留空

#### 方法 2：通过启动设置（Launch Settings）

如果项目属性中没有调试选项，可以创建 `launchSettings.json`：

1. 在 `Editor` 文件夹下创建 `Properties` 文件夹（如果不存在）
2. 创建 `launchSettings.json` 文件：

```json
{
  "profiles": {
    "FirstEngineEditor": {
      "commandName": "Project",
      "nativeDebugging": true,
      "workingDirectory": "$(OutDir)"
    }
  }
}
```

#### 方法 3：通过 .csproj 文件直接配置

如果上述方法都不行，可以直接编辑 `.csproj` 文件。我已经在项目文件中添加了必要的配置。

### 3.4 验证输出路径

确保项目的输出路径正确：

1. 右键项目 → 属性
2. 选择**"生成"**（Build）标签
3. 检查**"输出路径"**（Output path）：
   - Debug: `bin\Debug\net8.0-windows\`
   - Release: `bin\Release\net8.0-windows\`

## 步骤 4：解决"找不到指定文件"错误

如果设置启动项目后提示"Unable to Start program 系统找不到指定文件"，按以下步骤排查：

### 4.1 检查可执行文件是否存在

```powershell
# 检查 Debug 版本
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"

# 检查 Release 版本
Test-Path "Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe"
```

### 4.2 手动构建项目

如果文件不存在，手动构建：

```powershell
cd Editor
dotnet build -c Debug
```

### 4.3 检查项目配置

1. 在 Visual Studio 中，确保选择了正确的配置：
   - 顶部工具栏：选择 **Debug** 和 **x64**
   - 不是 **Any CPU**（WPF 项目需要 x64）

2. 检查项目属性：
   - 右键项目 → 属性 → 生成
   - **平台目标**（Platform target）：应该是 **x64**

### 4.4 清理并重新构建

```powershell
cd Editor
dotnet clean
dotnet build -c Debug
```

或者在 Visual Studio 中：
- 生成 → 清理解决方案
- 生成 → 重新生成解决方案

## 步骤 5：设置 C++ 调试（混合模式调试）

要同时调试 C# 和 C++ 代码：

### 5.1 设置 FirstEngine_Core 项目

1. 在解决方案资源管理器中，找到 `FirstEngine_Core` 项目
2. 右键 → 属性
3. 选择**"调试"**（Debugging）标签
4. 配置以下设置：
   - **配置类型**：动态库 (.dll)
   - **命令**（Command）：浏览并选择 `Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe`
   - **命令参数**：留空
   - **工作目录**（Working Directory）：`$(SolutionDir)Editor\bin\Debug\net8.0-windows`
   - **附加**（Attach）：选择"是"或"自动确定要调试的代码类型"

## 步骤 6：开始调试

### 6.1 设置断点

在以下位置设置断点：

**C# 代码：**
- `App.xaml.cs` 的 `OnStartup` 方法
- `RenderEngineService.cs` 的 `Initialize` 方法

**C++ 代码：**
- `EditorAPI.cpp` 的 `EditorAPI_CreateEngine` 函数
- `RenderContext.cpp` 的初始化方法

### 6.2 启动调试

1. 按 **F5** 或点击"开始调试"按钮
2. 如果一切正常，程序应该启动并在断点处停止

### 6.3 如果仍然无法启动

检查以下内容：

1. **检查输出窗口**：
   - 视图 → 输出（或 Ctrl+Alt+O）
   - 选择"调试"或"生成"输出源
   - 查看错误信息

2. **检查 DLL 依赖**：
   ```powershell
   # 确保这些 DLL 在 Editor\bin\Debug\net8.0-windows\ 目录下
   FirstEngine_Core.dll
   FirstEngine_RHI.dll
   # 以及其他依赖 DLL
   ```

3. **手动运行可执行文件**：
   ```powershell
   cd Editor\bin\Debug\net8.0-windows
   .\FirstEngineEditor.exe
   ```
   如果手动运行成功，说明是 Visual Studio 配置问题。

## 快速检查清单

在开始调试前，确保：

- [ ] 项目已成功构建（没有编译错误）
- [ ] `FirstEngineEditor.exe` 存在于 `Editor\bin\Debug\net8.0-windows\` 目录
- [ ] 项目配置设置为 **Debug** 和 **x64**
- [ ] 在项目属性中启用了"启用本机代码调试"
- [ ] 所有必需的 DLL 都在输出目录中
- [ ] 工作目录设置正确

## 常见错误和解决方案

### 错误 1："系统找不到指定文件"

**原因**：可执行文件不存在或路径不正确

**解决**：
1. 构建项目：`dotnet build -c Debug`
2. 检查输出路径配置
3. 确保选择了正确的配置（Debug/x64）

### 错误 2：找不到"调试"属性页

**原因**：Visual Studio 版本或项目类型问题

**解决**：
1. 确保使用 Visual Studio 2022
2. 确保项目是 WPF 应用程序（.NET SDK 项目）
3. 尝试通过 `.csproj.user` 文件配置（见下方）

### 错误 3：无法附加到进程

**原因**：权限问题或进程已运行

**解决**：
1. 以管理员权限运行 Visual Studio
2. 关闭所有 FirstEngineEditor 进程
3. 使用"附加到进程"而不是"启动"

## 备用方法：手动创建调试配置

如果 Visual Studio 的界面找不到调试设置，可以手动创建配置文件：

### 创建 `.csproj.user` 文件

在 `Editor` 目录下创建 `FirstEngineEditor.csproj.user` 文件：

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="Current" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <StartAction>Project</StartAction>
    <StartProgram>$(OutDir)$(TargetFileName)</StartProgram>
    <StartWorkingDirectory>$(OutDir)</StartWorkingDirectory>
    <EnableUnmanagedDebugging>true</EnableUnmanagedDebugging>
  </PropertyGroup>
</Project>
```

这个文件会被 Visual Studio 自动识别。

## 验证调试配置

设置完成后，验证配置：

1. 在 `App.xaml.cs` 的 `OnStartup` 方法中设置断点
2. 按 F5 启动
3. 如果断点被命中，说明 C# 调试正常
4. 在 C++ 代码中设置断点，如果也能命中，说明混合模式调试正常

## 需要帮助？

如果仍然无法设置调试，请提供：
1. Visual Studio 版本
2. 具体的错误消息
3. 项目构建是否成功
4. 可执行文件是否存在
