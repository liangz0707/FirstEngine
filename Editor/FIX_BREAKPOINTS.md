# 修复断点不生效问题

## 当前状态

✅ 程序可以正常启动
❌ 断点不生效（程序运行但不停止在断点处）

## 立即执行的步骤

### 步骤 1：清理并重新生成项目（必须！）

**重要**：确保使用最新的调试配置重新编译。

**在 Visual Studio 中**：
1. 右键 `FirstEngineEditor` 项目
2. **"清理"** (Clean)
3. 等待完成
4. **"重新生成"** (Rebuild)

**或在命令行**：
```powershell
cd Editor
dotnet clean
dotnet build -c Debug --no-incremental
```

### 步骤 2：验证符号文件

检查 `.pdb` 文件是否存在：

```powershell
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb"
```

如果返回 `False`，说明符号文件未生成，需要重新生成项目。

### 步骤 3：在正确的位置设置断点

**测试位置 1：App.xaml.cs - OnStartup 方法**

```csharp
protected override void OnStartup(StartupEventArgs e)
{
    var debugTest = 123;  // 👈 在这里设置断点（第 10 行）
    base.OnStartup(e);
    
    // Initialize services
    Services.ServiceLocator.Initialize();
}
```

**测试位置 2：MainWindow.xaml.cs - 构造函数**

```csharp
public MainWindow()
{
    var test = "debug";  // 👈 在这里设置断点（第 13 行之后）
    InitializeComponent();
    _projectManager = ServiceLocator.Get<IProjectManager>();
}
```

### 步骤 4：检查 Visual Studio 调试器设置

1. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 常规 (General)**：
   - ✅ **启用 .NET Framework 源代码步进**
   - ⚠️ **要求源文件与原始版本完全匹配** - 如果启用，确保源代码未修改
   - ✅ **启用源服务器支持**

2. **调试 (Debugging) → 符号 (Symbols)**：
   - 确保本地符号缓存路径正确

### 步骤 5：启动调试并检查模块窗口

1. **按 F5 启动调试**

2. **打开模块窗口**：
   - 调试 (Debug) → 窗口 (Windows) → 模块 (Modules)
   - 或者按 `Ctrl+Alt+U`

3. **查找 `FirstEngineEditor.exe`**：
   - 检查 **"符号状态"** 列
   - ✅ **"已加载符号"** = 正常
   - ⚠️ **"无法查找或打开 PDB 文件"** = 需要手动加载

4. **如果符号未加载**：
   - 右键 `FirstEngineEditor.exe`
   - 选择 **"加载符号"** (Load Symbols)
   - 浏览到：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb`

### 步骤 6：检查输出窗口

1. **查看 (View) → 输出 (Output)**
2. **选择 "调试" (Debug) 输出**
3. **查找以下信息**：
   - `'FirstEngineEditor.exe' (Managed): 已加载符号` ✅
   - `符号未加载` 或 `无法查找 PDB 文件` ❌

## 常见问题及解决方案

### 问题 1：断点显示为空心圆圈

**症状**：断点是空心的，鼠标悬停显示 "当前不会命中断点。还没有为此文档加载任何符号。"

**原因**：
- 符号文件未生成
- 符号文件不匹配
- 代码已优化

**解决**：
1. 清理并重新生成项目
2. 确认 Debug 配置（不是 Release）
3. 确认 `.csproj` 中 `Optimize=false`
4. 检查 `.pdb` 文件是否存在

### 问题 2：断点是实心但程序不停止

**症状**：断点是实心红色，但程序运行时不停止。

**可能原因**：
1. **代码路径未执行** - 断点设置的代码可能不会被执行
2. **条件断点** - 断点可能设置了条件，条件不满足
3. **调试器未正确附加** - 调试器可能没有正确附加到进程

**解决**：
1. **确认代码会被执行**：
   - 在 `OnStartup` 方法的最开始设置断点
   - 或者在 `MainWindow` 构造函数中设置断点
   - 添加 `Console.WriteLine("Debug test");` 确认代码执行

2. **检查断点条件**：
   - 右键断点 → 检查是否有条件
   - 如果有条件，删除条件或确保条件满足

3. **检查调试器附加**：
   - 查看 → 输出 → 调试
   - 查找 "程序 '[xxxx] FirstEngineEditor.exe' 已启动"
   - 查找 "已附加调试器"

### 问题 3：符号未加载

**症状**：在模块窗口中显示 "无法查找或打开 PDB 文件"

**解决**：
1. **手动加载符号**（见步骤 5）
2. **检查文件路径**：
   ```powershell
   Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb"
   ```
3. **重新生成项目**，确保 `.pdb` 文件是最新的

### 问题 4：.NET 8.0 特定问题

**症状**：在 .NET 8.0 中，某些调试功能可能需要额外配置。

**解决**：
1. 确认 Visual Studio 版本支持 .NET 8.0
2. 确认已安装 .NET 8.0 SDK
3. 尝试使用 `DebugType=portable` 而不是 `full`（在某些情况下）

## 测试断点的最佳实践

### 方法 1：使用 Console.WriteLine 验证代码执行

```csharp
protected override void OnStartup(StartupEventArgs e)
{
    Console.WriteLine("OnStartup called!");  // 如果看到这行输出，说明代码执行了
    var debugTest = 123;  // 👈 在这里设置断点
    base.OnStartup(e);
    // ...
}
```

### 方法 2：使用 System.Diagnostics.Debugger.Break()

```csharp
protected override void OnStartup(StartupEventArgs e)
{
    System.Diagnostics.Debugger.Break();  // 强制中断
    base.OnStartup(e);
    // ...
}
```

如果 `Debugger.Break()` 可以工作，说明调试器已正确附加，问题可能是符号文件。

## 验证清单

- [ ] 项目已清理并重新生成
- [ ] `.pdb` 文件存在且时间戳与 `.exe` 匹配
- [ ] Visual Studio 配置：**Debug x64**
- [ ] 断点是**实心红色圆圈**（不是空心）
- [ ] 已启用本机代码调试（`.csproj.user` 中 `EnableUnmanagedDebugging=true`）
- [ ] 代码未优化（`.csproj` 中 `Optimize=false`）
- [ ] 符号已加载（检查模块窗口）
- [ ] 调试器已附加（检查输出窗口）

## 如果所有方法都失败

1. **完全重启 Visual Studio**
2. **删除 `.vs` 文件夹**（在解决方案目录 `build/`）
3. **删除 `bin` 和 `obj` 文件夹**（在 `Editor/` 目录）
4. **重新打开解决方案**
5. **重新生成项目**
6. **再次尝试调试**

## 调试输出示例

正常情况下的输出窗口应该显示：
```
'FirstEngineEditor.exe' (Managed): 已加载 'C:\Program Files\dotnet\shared\Microsoft.NETCore.App\8.0.0\System.Private.CoreLib.dll'，已加载符号。
'FirstEngineEditor.exe' (Managed): 已加载 'G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe'，已加载符号。
程序 '[xxxx] FirstEngineEditor.exe' 已启动。
```

如果看到 "符号未加载" 或 "无法查找 PDB 文件"，需要按照步骤 5 手动加载符号。
