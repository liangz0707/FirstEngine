# 断点不生效 - 故障排除指南

## 当前状态

✅ `.pdb` 文件已存在（`FirstEngineEditor.pdb`，189KB）
✅ 调试配置已更新（`Optimize=false`，`DebugType=full`）
✅ 混合模式调试已启用

## 立即执行的步骤

### 步骤 1：清理并重新生成（必须！）

**重要**：更新配置后，必须清理并重新生成项目！

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

### 步骤 2：验证断点设置

1. **在 `App.xaml.cs` 中设置断点**：
   ```csharp
   protected override void OnStartup(StartupEventArgs e)
   {
       var debugTest = 123;  // 👈 在这里设置断点（第 10 行左右）
       base.OnStartup(e);
       
       // Initialize services
       Services.ServiceLocator.Initialize();
   }
   ```

2. **检查断点状态**：
   - ✅ **实心红色圆圈** = 断点已绑定，应该可以工作
   - ⚠️ **空心圆圈** = 断点未绑定，需要检查

### 步骤 3：检查 Visual Studio 调试器设置

1. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 常规 (General)**：
   - ✅ **启用 .NET Framework 源代码步进**
   - ✅ **启用源服务器支持**
   - ⚠️ **要求源文件与原始版本完全匹配** - 如果启用，确保源代码未修改

2. **调试 (Debugging) → 符号 (Symbols)**：
   - 确保本地符号缓存路径正确
   - 可以临时启用 "Microsoft 符号服务器" 进行测试

### 步骤 4：启动调试并检查

1. **按 F5 启动调试**

2. **查看输出窗口**：
   - 查看 (View) → 输出 (Output)
   - 选择 **"调试"** (Debug)
   - 查找以下信息：
     - `FirstEngineEditor.exe' (Managed): 已加载符号`
     - 或 `符号未加载` 的警告

3. **查看模块窗口**：
   - 调试 (Debug) → 窗口 (Windows) → 模块 (Modules)
   - 找到 `FirstEngineEditor.exe`
   - 检查 **"符号状态"** 列：
     - ✅ **"已加载符号"** = 正常
     - ⚠️ **"无法查找或打开 PDB 文件"** = 需要手动加载

### 步骤 5：如果符号未加载，手动加载

1. 在模块窗口中，右键 `FirstEngineEditor.exe`
2. 选择 **"加载符号"** (Load Symbols)
3. 浏览到：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb`
4. 点击 **"打开"**

## 常见问题及解决方案

### 问题 1：断点显示为空心圆圈

**症状**：断点是空心的，鼠标悬停显示 "当前不会命中断点。还没有为此文档加载任何符号。"

**原因**：
- 代码已优化
- 符号文件不匹配
- 源代码与编译的代码不一致

**解决**：
1. 确认 Visual Studio 配置是 **Debug**，不是 Release
2. 清理并重新生成项目
3. 检查 `.csproj` 中 `Optimize=false`
4. 确认 `.pdb` 文件与 `.exe` 文件时间戳匹配

### 问题 2：断点是实心但程序不停止

**症状**：断点是实心红色，但程序运行时不停止。

**原因**：
- 代码路径未执行
- 条件断点条件不满足
- 调试器未正确附加

**解决**：
1. **确认代码会被执行**：
   - 在 `OnStartup` 方法的最开始设置断点
   - 或者在 `MainWindow` 构造函数中设置断点

2. **检查断点条件**：
   - 右键断点 → 检查是否有条件
   - 如果有条件，删除条件或确保条件满足

3. **检查调试器附加**：
   - 查看 → 输出 → 调试
   - 查找 "程序 '[xxxx] FirstEngineEditor.exe' 已启动"
   - 查找 "已附加调试器"

### 问题 3：提示"符号未加载"

**症状**：在模块窗口中显示 "无法查找或打开 PDB 文件"

**解决**：
1. **手动加载符号**（见步骤 5）
2. **检查文件路径**：
   ```powershell
   Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb"
   ```
3. **重新生成项目**，确保 `.pdb` 文件是最新的

### 问题 4：可以调试 C# 但无法调试 C++ DLL

**症状**：C# 断点可以工作，但 C++ 代码断点不工作

**解决**：
1. 确认已启用混合模式调试（`.csproj.user` 中 `EnableUnmanagedDebugging=true`）
2. 确认 C++ DLL 也是 Debug 配置编译
3. 确认 C++ DLL 的 `.pdb` 文件存在

## 测试断点的最佳位置

### 位置 1：App.xaml.cs - OnStartup

```csharp
protected override void OnStartup(StartupEventArgs e)
{
    var test = "This should break";  // 👈 设置断点
    base.OnStartup(e);
    Services.ServiceLocator.Initialize();
}
```

### 位置 2：MainWindow.xaml.cs - 构造函数

```csharp
public MainWindow()
{
    var test = 123;  // 👈 设置断点
    InitializeComponent();
    // ...
}
```

### 位置 3：MainWindow.xaml.cs - Loaded 事件

```csharp
private void MainWindow_Loaded(object sender, RoutedEventArgs e)
{
    var test = "Loaded";  // 👈 设置断点
    // ...
}
```

## 验证清单

- [ ] 项目已清理并重新生成
- [ ] `.pdb` 文件存在且时间戳与 `.exe` 匹配
- [ ] Visual Studio 配置：**Debug x64**
- [ ] 断点是**实心红色圆圈**
- [ ] 已启用本机代码调试
- [ ] 代码未优化（`Optimize=false`）
- [ ] 符号已加载（检查模块窗口）
- [ ] 调试器已附加（检查输出窗口）

## 如果所有方法都失败

1. **完全重启 Visual Studio**
2. **删除 `.vs` 文件夹**（在解决方案目录）
3. **删除 `bin` 和 `obj` 文件夹**
4. **重新打开解决方案**
5. **重新生成项目**
6. **再次尝试调试**

## 调试输出示例

正常情况下的输出窗口应该显示：
```
'FirstEngineEditor.exe' (Managed): 已加载 'C:\Windows\Microsoft.NET\Framework64\v4.0.30319\mscorlib.dll'，已加载符号。
'FirstEngineEditor.exe' (Managed): 已加载 'G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe'，已加载符号。
程序 '[xxxx] FirstEngineEditor.exe' 已启动。
```

如果看到 "符号未加载" 或 "无法查找 PDB 文件"，需要按照步骤 5 手动加载符号。
