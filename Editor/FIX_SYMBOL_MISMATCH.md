# 修复符号文件不匹配问题

## 问题

选择"加载符号"时提示：
```
A Matching symbol file was not found in this folder
```

## 原因

`.pdb` 文件与 `.exe` 文件不匹配，可能因为：
1. `.pdb` 和 `.exe` 的时间戳不一致
2. 代码被重新编译但符号文件未更新
3. 存在多个版本的 `.exe` 或 `.pdb` 文件

## 解决方案

### 步骤 1：清理并完全重新生成项目（最重要！）

**在 Visual Studio 中**：
1. 右键 `FirstEngineEditor` 项目
2. **"清理"** (Clean)
3. 等待完成
4. **"重新生成"** (Rebuild)

**或在命令行**：
```powershell
cd Editor
dotnet clean
Remove-Item -Recurse -Force bin, obj -ErrorAction SilentlyContinue
dotnet build -c Debug --no-incremental
```

### 步骤 2：验证文件时间戳匹配

检查 `.exe` 和 `.pdb` 文件的时间戳是否一致：

```powershell
cd Editor
$exe = Get-Item "bin\Debug\net8.0-windows\FirstEngineEditor.exe"
$pdb = Get-Item "bin\Debug\net8.0-windows\FirstEngineEditor.pdb"
Write-Host "EXE: $($exe.LastWriteTime)"
Write-Host "PDB: $($pdb.LastWriteTime)"
```

如果时间戳不一致，说明需要重新生成。

### 步骤 3：检查是否有多个版本的文件

确保只有一个版本的 `.exe` 和 `.pdb` 文件：

```powershell
Get-ChildItem -Recurse "bin\Debug\net8.0-windows\FirstEngineEditor.*" | Select-Object FullName, LastWriteTime
```

### 步骤 4：在 Visual Studio 中重新加载符号

1. **关闭正在运行的程序**（如果正在调试）

2. **清理并重新生成项目**（见步骤 1）

3. **重新启动调试**（F5）

4. **打开模块窗口**：
   - 调试 → 窗口 → 模块（`Ctrl+Alt+U`）

5. **查找 `FirstEngineEditor.exe`**

6. **如果仍然显示符号未加载**：
   - 右键 `FirstEngineEditor.exe`
   - 选择 **"符号设置"** (Symbol Settings)
   - 或者选择 **"加载符号"** (Load Symbols)
   - 这次应该可以找到匹配的符号文件

### 步骤 5：配置符号搜索路径

如果仍然不工作，手动配置符号搜索路径：

1. **工具 (Tools) → 选项 (Options) → 调试 (Debugging) → 符号 (Symbols)**

2. **添加符号文件位置**：
   - 点击 **"新建文件夹"** (New Folder)
   - 添加：`G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows`
   - 点击 **"确定"**

3. **清除符号缓存**（可选）：
   - 点击 **"清空符号缓存"** (Empty Symbol Cache)
   - 点击 **"确定"**

4. **重新启动调试**

### 步骤 6：使用 Debugger.Break() 验证调试器

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

## 常见问题

### Q: 为什么会出现符号文件不匹配？

A: 可能的原因：
- 代码被修改后只重新编译了 `.exe`，没有重新生成 `.pdb`
- 使用了增量编译，导致文件版本不一致
- 存在多个构建输出目录

### Q: 如何确保符号文件匹配？

A: 
1. 总是使用 **"重新生成"** (Rebuild)，而不是只编译
2. 清理 `bin` 和 `obj` 文件夹
3. 使用 `--no-incremental` 标志强制完全重新编译

### Q: 可以禁用符号验证吗？

A: 不推荐，但可以：
1. 工具 → 选项 → 调试 → 常规
2. 取消勾选 **"要求源文件与原始版本完全匹配"**
3. 但这可能导致调试信息不准确

## 验证清单

- [ ] 项目已完全清理（删除 `bin` 和 `obj` 文件夹）
- [ ] 项目已重新生成（不是只编译）
- [ ] `.exe` 和 `.pdb` 文件时间戳匹配
- [ ] 只有一个版本的 `.exe` 和 `.pdb` 文件
- [ ] 符号搜索路径已配置
- [ ] 已重新启动调试

## 如果仍然不工作

### 方法 1：完全重置

1. **关闭 Visual Studio**
2. **删除以下文件夹**：
   - `Editor\bin`
   - `Editor\obj`
   - `build\.vs`（如果存在）
3. **重新打开 Visual Studio**
4. **重新生成项目**
5. **测试调试**

### 方法 2：检查 .NET SDK 版本

确保使用的 .NET SDK 版本一致：

```powershell
dotnet --version
```

在项目文件中确认目标框架版本匹配。

### 方法 3：使用 DebugType=portable

尝试修改 `.csproj` 文件，使用便携式符号：

```xml
<PropertyGroup Condition="'$(Configuration)'=='Debug'">
  <DebugType>portable</DebugType>  <!-- 改为 portable -->
  <DebugSymbols>true</DebugSymbols>
  <Optimize>false</Optimize>
</PropertyGroup>
```

然后重新生成项目。

## 快速修复命令

一键清理并重新生成：

```powershell
cd Editor
dotnet clean
Remove-Item -Recurse -Force bin, obj -ErrorAction SilentlyContinue
dotnet build -c Debug --no-incremental
Write-Host "✅ 项目已重新生成，符号文件应该已匹配"
```
