# 修复 DLL 加载问题

## 问题

C# 编辑器无法加载 `FirstEngine_Core.dll`，错误信息：
```
Unable to load DLL 'FirstEngine_Core.dll' or one of its dependencies: 找不到指定的模块。 (0x8007007E)
```

## 原因

1. **DLL 名称不匹配**：
   - Debug 配置下，CMake 生成的 DLL 名称是 `FirstEngine_Cored.dll`（带 'd' 后缀）
   - 但 C# 代码期望的是 `FirstEngine_Core.dll`（不带 'd' 后缀）

2. **DLL 依赖项缺失**：
   - `FirstEngine_Core.dll` 可能依赖其他 DLL（如 GLFW、Vulkan 等）
   - 这些依赖 DLL 可能不在编辑器输出目录中

## 已完成的修复

### 1. 修复 DLL 名称问题

在 `Editor/CMakeLists.txt` 中，添加了 PowerShell 命令来创建不带 'd' 后缀的副本：

- Debug 配置下，复制 `FirstEngine_Cored.dll` 到 `FirstEngine_Core.dll`
- 同样处理 `FirstEngine_RHId.dll` 和 `FirstEngine_Shaderd.dll`

### 2. 确保 DLL 被复制到编辑器输出目录

CMake 的 `POST_BUILD` 命令会：
1. 复制所有必需的 DLL 到编辑器输出目录
2. 在 Debug 配置下，创建不带 'd' 后缀的副本

## 现在请执行

### 步骤 1：重新生成 CMake 配置

在 Visual Studio 中：
1. 右键解决方案 → **"重新生成解决方案"** (Rebuild Solution)

或在命令行：
```powershell
cd build
cmake --build . --config Debug --target FirstEngineEditor
```

### 步骤 2：验证 DLL 文件

检查编辑器输出目录中是否有以下文件：

```powershell
cd Editor\bin\Debug\net8.0-windows
dir *.dll
```

**应该看到**：
- `FirstEngine_Cored.dll`（Debug 版本，带 'd' 后缀）
- `FirstEngine_Core.dll`（不带 'd' 后缀，供 C# 使用）
- `FirstEngine_RHId.dll` 和 `FirstEngine_RHI.dll`
- `FirstEngine_Shaderd.dll` 和 `FirstEngine_Shader.dll`
- `vulkan-1.dll`

### 步骤 3：检查 DLL 依赖项

如果仍然无法加载，可能是缺少依赖 DLL。检查 `FirstEngine_Core.dll` 的依赖项：

1. **使用 Dependency Walker** 或 **Dependencies** 工具：
   - 下载 Dependencies: https://github.com/lucasg/Dependencies
   - 打开 `Editor\bin\Debug\net8.0-windows\FirstEngine_Core.dll`
   - 查看缺失的 DLL

2. **常见缺失的 DLL**：
   - `glfw3.dll` - GLFW 库
   - `vulkan-1.dll` - Vulkan 运行时（应该已经复制）
   - Visual C++ 运行时（通常已安装）

### 步骤 4：手动复制缺失的 DLL

如果发现缺失的 DLL：

1. **GLFW DLL**：
   ```powershell
   # 如果 GLFW 是动态链接的，需要复制 glfw3.dll
   # 检查 build\lib\Debug\ 或 build\bin\Debug\ 中是否有 glfw3.dll
   Copy-Item "build\bin\Debug\glfw3.dll" "Editor\bin\Debug\net8.0-windows\" -Force
   ```

2. **Visual C++ 运行时**：
   - 确保安装了 Visual C++ Redistributable
   - 下载：https://aka.ms/vs/17/release/vc_redist.x64.exe

## 如果仍然有问题

### 方法 1：检查 DLL 路径

确保 C# 编辑器在正确的目录中查找 DLL：

1. **检查工作目录**：
   - 在 `RenderEngineService.cs` 的 `Initialize` 方法中添加：
     ```csharp
     System.Diagnostics.Debug.WriteLine($"Current directory: {Environment.CurrentDirectory}");
     System.Diagnostics.Debug.WriteLine($"DLL search path: {Environment.GetEnvironmentVariable("PATH")}");
     ```

2. **使用绝对路径**（临时测试）：
   ```csharp
   [DllImport(@"G:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows\FirstEngine_Core.dll", ...)]
   ```

### 方法 2：使用 SetDllDirectory

在 C# 代码中设置 DLL 搜索路径：

```csharp
[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
static extern bool SetDllDirectory(string lpPathName);

// 在 Initialize 方法中：
string dllPath = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
SetDllDirectory(dllPath);
```

### 方法 3：检查 DLL 架构

确保 DLL 架构匹配：
- C# 编辑器是 **x64**
- C++ DLL 也应该是 **x64**

检查：
```powershell
dumpbin /headers "Editor\bin\Debug\net8.0-windows\FirstEngine_Core.dll" | Select-String "machine"
```

应该显示 `8664 machine (x64)`。

## 验证清单

- [ ] 重新生成了 CMake 配置
- [ ] 重新编译了 C++ 项目
- [ ] 检查了编辑器输出目录中的 DLL 文件
- [ ] 确认有 `FirstEngine_Core.dll`（不带 'd' 后缀）
- [ ] 确认有 `vulkan-1.dll`
- [ ] 检查了 DLL 依赖项（使用 Dependencies 工具）
- [ ] 确认 DLL 架构是 x64
- [ ] 尝试运行编辑器

## 快速修复脚本

如果手动复制 DLL，可以使用以下 PowerShell 脚本：

```powershell
# 复制 DLL 到编辑器输出目录
$sourceDir = "build\bin\Debug"
$destDir = "Editor\bin\Debug\net8.0-windows"

# 复制 DLL（带 'd' 后缀）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Cored.dll" -Force
Copy-Item "$sourceDir\FirstEngine_RHId.dll" "$destDir\FirstEngine_RHId.dll" -Force
Copy-Item "$sourceDir\FirstEngine_Shaderd.dll" "$destDir\FirstEngine_Shaderd.dll" -Force

# 创建不带 'd' 后缀的副本（供 C# 使用）
Copy-Item "$sourceDir\FirstEngine_Cored.dll" "$destDir\FirstEngine_Core.dll" -Force
Copy-Item "$sourceDir\FirstEngine_RHId.dll" "$destDir\FirstEngine_RHI.dll" -Force
Copy-Item "$sourceDir\FirstEngine_Shaderd.dll" "$destDir\FirstEngine_Shader.dll" -Force

# 复制 vulkan-1.dll
if (Test-Path "$sourceDir\vulkan-1.dll") {
    Copy-Item "$sourceDir\vulkan-1.dll" "$destDir\vulkan-1.dll" -Force
}

Write-Host "DLLs copied successfully!"
```
