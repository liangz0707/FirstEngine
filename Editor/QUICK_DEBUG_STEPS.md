# 快速调试设置步骤

## 问题：找不到调试设置选项

在 Visual Studio 2022 中，对于 .NET SDK 项目（如 FirstEngineEditor），调试设置的界面可能不同。

## 解决方案

### 方法 1：使用项目属性（如果能看到）

1. **打开项目**
   - 在 Visual Studio 中打开 `build/FirstEngine.sln`
   - 或直接打开 `Editor/FirstEngineEditor.csproj`

2. **打开项目属性**
   - 右键点击 `FirstEngineEditor` 项目
   - 选择**"属性"**（Properties）

3. **查找调试设置**
   - 在属性窗口中，查找以下标签页：
     - **"调试"**（Debug）
     - **"调试"**（Debugging）  
     - **"启动"**（Launch）
   
   **注意**：在 Visual Studio 2022 中，可能显示为：
   - 左侧导航栏中的**"调试"**选项
   - 或者顶部标签页中的**"调试"**

4. **启用本机代码调试**
   - 找到**"启用本机代码调试"**（Enable native code debugging）
   - 勾选此选项 ✅

### 方法 2：使用 .csproj.user 文件（已创建）

我已经创建了 `FirstEngineEditor.csproj.user` 文件，它会自动配置调试设置。

**如果 Visual Studio 仍然找不到设置，请：**

1. **关闭 Visual Studio**
2. **重新打开项目**
3. **检查是否生效**

### 方法 3：通过 launchSettings.json（备用）

如果上述方法都不行，创建 `Properties/launchSettings.json`：

1. 在 `Editor` 文件夹下创建 `Properties` 文件夹
2. 创建 `launchSettings.json`：

```json
{
  "profiles": {
    "FirstEngineEditor": {
      "commandName": "Project",
      "nativeDebugging": true
    }
  }
}
```

## 解决"找不到指定文件"错误

### 步骤 1：确保项目已构建

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine\Editor
dotnet build -c Debug
```

### 步骤 2：验证文件存在

```powershell
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe"
```

应该返回 `True`。

### 步骤 3：检查 Visual Studio 配置

1. **确保选择了正确的配置**
   - 顶部工具栏：选择 **Debug**（不是 Release）
   - 选择 **x64**（不是 Any CPU）

2. **清理并重新构建**
   - 生成 → 清理解决方案
   - 生成 → 重新生成解决方案

### 步骤 4：手动设置启动项目

1. 在解决方案资源管理器中
2. 右键 `FirstEngineEditor` 项目
3. 选择**"设为启动项目"**

如果仍然提示"找不到文件"，检查：
- 项目是否成功构建（查看输出窗口）
- 输出路径是否正确
- 是否有编译错误

## 快速验证

运行以下命令验证一切正常：

```powershell
# 1. 构建项目
cd g:\AIHUMAN\WorkSpace\FirstEngine\Editor
dotnet build -c Debug

# 2. 检查文件
Test-Path "bin\Debug\net8.0-windows\FirstEngineEditor.exe"

# 3. 手动运行（测试）
.\bin\Debug\net8.0-windows\FirstEngineEditor.exe
```

如果手动运行成功，说明程序本身没问题，只是 Visual Studio 配置问题。

## 在 Visual Studio 中设置调试的详细步骤

### 步骤 1：打开项目

- 打开 `build/FirstEngine.sln` 或 `Editor/FirstEngineEditor.csproj`

### 步骤 2：设置启动项目

- 解决方案资源管理器 → 右键 `FirstEngineEditor` → **设为启动项目**

### 步骤 3：配置调试（Visual Studio 2022）

**选项 A：通过属性窗口**

1. 右键项目 → **属性**
2. 在属性窗口中，查找：
   - 如果看到**"调试"**标签，点击它
   - 如果看到**"启动"**标签，点击它
   - 如果看到**"调试"**部分，展开它

3. 查找以下选项：
   - **启用本机代码调试**（Enable native code debugging）
   - **启用本机调试**（Enable native debugging）
   - **启用非托管代码调试**（Enable unmanaged code debugging）

**选项 B：通过调试菜单**

1. 调试 → **FirstEngineEditor 调试属性**（如果有）
2. 或：调试 → **选项** → **调试** → **常规**
3. 勾选**"启用本机代码调试"**

**选项 C：直接编辑 .csproj.user 文件**

我已经创建了 `FirstEngineEditor.csproj.user` 文件，它应该会自动生效。

### 步骤 4：开始调试

1. 在代码中设置断点（F9）
2. 按 F5 启动调试
3. 如果断点被命中，说明调试正常

## 如果仍然无法设置

### 检查 Visual Studio 版本

确保使用 **Visual Studio 2022**（不是 2019 或更早版本）

### 检查项目类型

确保项目是 **.NET SDK 项目**（不是旧式 .NET Framework 项目）

### 手动编辑配置

如果所有方法都失败，可以直接编辑 `.csproj` 文件，添加：

```xml
<PropertyGroup>
  <EnableUnmanagedDebugging>true</EnableUnmanagedDebugging>
</PropertyGroup>
```

## 测试调试是否工作

1. 在 `App.xaml.cs` 的 `OnStartup` 方法中设置断点
2. 按 F5
3. 如果程序在断点处停止，说明 C# 调试正常
4. 在 `EditorAPI.cpp` 中设置断点
5. 如果也能停止，说明混合模式调试正常

## 需要更多帮助？

如果仍然有问题，请提供：
1. Visual Studio 的完整版本号
2. 项目属性窗口中能看到哪些标签页
3. 具体的错误消息截图
