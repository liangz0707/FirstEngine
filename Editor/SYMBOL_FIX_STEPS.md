# 修复符号文件不匹配 - 详细步骤

## 问题

选择"加载符号"时提示：
```
A Matching symbol file was not found in this folder
```

## 已完成的修复

1. ✅ 已将 `DebugType` 从 `full` 改为 `portable`（更兼容）
2. ✅ 创建了清理和重新生成脚本

## 立即执行的步骤

### 步骤 1：运行清理脚本

在 PowerShell 中运行：

```powershell
cd Editor
.\clean_and_rebuild.ps1
```

或者手动执行：

```powershell
cd Editor
dotnet clean
Remove-Item -Recurse -Force bin, obj -ErrorAction SilentlyContinue
dotnet build -c Debug --no-incremental
```

### 步骤 2：配置 Visual Studio 符号搜索路径

1. **打开 Visual Studio**

2. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 符号 (Symbols)**

3. **添加符号搜索路径**：
   - 点击 **"新建文件夹"** (New Folder) 或 **"+"** 按钮
   - 添加路径：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows`
   - 点击 **"确定"**

4. **清除符号缓存**（重要！）：
   - 点击 **"清空符号缓存"** (Empty Symbol Cache)
   - 点击 **"确定"**

5. **确保以下选项已启用**：
   - ✅ **Microsoft 符号服务器**（可选，用于系统库）
   - ✅ **NuGet.org 符号服务器**（可选）

### 步骤 3：关闭并重新打开 Visual Studio

**重要**：配置更改后，必须完全关闭并重新打开 Visual Studio。

### 步骤 4：重新启动调试

1. **设置断点**（在 `App.xaml.cs` 第 11 行）

2. **按 F5 启动调试**

3. **打开模块窗口**：
   - 调试 → 窗口 → 模块（`Ctrl+Alt+U`）

4. **检查符号状态**：
   - 找到 `FirstEngineEditor.exe`
   - 检查 **"符号状态"** 列
   - 应该显示 **"已加载符号"**

### 步骤 5：如果仍然不工作

#### 方法 A：手动指定符号文件

1. 在模块窗口中，右键 `FirstEngineEditor.exe`
2. 选择 **"符号设置"** (Symbol Settings)
3. 在 **"符号文件(.pdb)位置"** 中，点击 **"浏览"**
4. 选择：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngineEditor.pdb`
5. 点击 **"确定"**

#### 方法 B：使用 Debugger.Break() 验证

如果符号加载仍然有问题，使用 `Debugger.Break()` 验证调试器是否工作：

```csharp
protected override void OnStartup(StartupEventArgs e)
{
    System.Diagnostics.Debugger.Break();  // 强制中断，不依赖符号文件
    var debugTest = 123;
    base.OnStartup(e);
    // ...
}
```

如果 `Debugger.Break()` 可以工作，说明调试器已附加，问题确实是符号文件。

#### 方法 C：检查 .NET 版本

确保 Visual Studio 和项目使用相同的 .NET 版本：

```powershell
dotnet --version
```

在 Visual Studio 中：
- 工具 → 选项 → 项目和解决方案 → .NET Core
- 检查 .NET SDK 版本

## 为什么改为 portable？

`DebugType=portable` 使用跨平台的便携式 PDB 格式，相比 `full` 格式：
- ✅ 更兼容不同工具
- ✅ 文件更小
- ✅ 在某些情况下更容易被 Visual Studio 识别

如果 `portable` 仍然不工作，可以尝试改回 `full` 或使用 `embedded`（将符号嵌入到 DLL 中）。

## 验证清单

- [ ] 项目已完全清理并重新生成
- [ ] `.exe` 和 `.pdb` 文件时间戳匹配
- [ ] Visual Studio 符号搜索路径已配置
- [ ] 符号缓存已清除
- [ ] Visual Studio 已重新打开
- [ ] 断点已设置
- [ ] 模块窗口中显示"已加载符号"

## 如果所有方法都失败

### 最后的手段：使用 embedded 符号

修改 `.csproj` 文件：

```xml
<PropertyGroup Condition="'$(Configuration)'=='Debug'">
  <DebugType>embedded</DebugType>  <!-- 将符号嵌入到 DLL 中 -->
  <DebugSymbols>true</DebugSymbols>
  <Optimize>false</Optimize>
</PropertyGroup>
```

然后重新生成项目。这样符号文件会嵌入到 `.dll` 中，不需要单独的 `.pdb` 文件。

## 快速命令

一键清理并重新生成：

```powershell
cd Editor
.\clean_and_rebuild.ps1
```
