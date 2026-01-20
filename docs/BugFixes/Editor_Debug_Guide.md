# FirstEngine 编辑器调试指南

## 概述

FirstEngine 编辑器是一个 C# WPF 应用程序，通过 P/Invoke 调用 C++ 渲染引擎（`FirstEngine_Core.dll`）。调试需要同时调试 C# 和 C++ 代码。

## 调试方法

### 方法 1：Visual Studio 混合模式调试（推荐）

这是最方便的方法，可以同时调试 C# 和 C++ 代码。

#### 步骤：

1. **打开解决方案**
   ```powershell
   # 在项目根目录
   cd g:\AIHUMAN\WorkSpace\FirstEngine
   # 使用 CMake 生成 Visual Studio 解决方案（如果还没有）
   cmake -B build -S .
   ```

2. **在 Visual Studio 中打开**
   - 打开 `build/FirstEngine.sln`
   - 或者直接打开 `Editor/FirstEngineEditor.csproj`

3. **设置启动项目**
   - 在解决方案资源管理器中，右键点击 `FirstEngineEditor` 项目
   - 选择"设为启动项目"

4. **配置调试设置**
   - 右键点击 `FirstEngineEditor` 项目 → 属性
   - 转到"调试"标签页
   - 确保以下设置：
     - **启动操作**：启动项目
     - **启用本机代码调试**：✅ 勾选（重要！）
     - **启用 SQL Server 调试**：可选

5. **设置 C++ 项目调试**
   - 在解决方案资源管理器中，右键点击 `FirstEngine_Core` 项目
   - 属性 → 调试
   - 确保：
     - **命令**：指向 `Editor/bin/Debug/net8.0-windows/FirstEngineEditor.exe`
     - **工作目录**：`$(SolutionDir)Editor\bin\Debug\net8.0-windows`
     - **附加到**：选择"自动确定要调试的代码类型"或"本机"

6. **开始调试**
   - 按 F5 或点击"开始调试"
   - Visual Studio 会自动附加到 C# 和 C++ 代码

### 方法 2：分别调试 C# 和 C++

如果混合模式调试有问题，可以分别调试。

#### 调试 C# 代码：

1. **设置断点**
   - 在 C# 代码中设置断点（例如 `App.xaml.cs`, `RenderEngineService.cs`）

2. **启动调试**
   - 在 Visual Studio 中按 F5
   - 或者使用命令行：
     ```powershell
     cd Editor
     dotnet run
     ```

3. **调试技巧**
   - 使用 `System.Diagnostics.Debug.WriteLine()` 输出调试信息
   - 检查输出窗口查看调试信息
   - 使用即时窗口（Ctrl+Alt+I）执行表达式

#### 调试 C++ 代码：

1. **使用 RenderDoc**
   - RenderDoc 已经集成到引擎中
   - 启动编辑器后，RenderDoc 会自动捕获帧
   - 可以在 RenderDoc 中查看渲染状态

2. **附加到进程**
   - 在 Visual Studio 中：调试 → 附加到进程
   - 选择 `FirstEngineEditor.exe`
   - 确保选择"本机"代码类型
   - 点击"附加"

3. **设置 C++ 断点**
   - 在 C++ 代码中设置断点
   - 例如：`EditorAPI.cpp`, `RenderContext.cpp`, `VulkanDevice.cpp`

4. **使用日志输出**
   - C++ 代码使用 `std::cout` 或 `std::cerr` 输出
   - 在 Visual Studio 的输出窗口中查看

### 方法 3：使用命令行调试

#### 启动编辑器：

```powershell
cd g:\AIHUMAN\WorkSpace\FirstEngine\Editor\bin\Debug\net8.0-windows
.\FirstEngineEditor.exe
```

#### 使用调试器：

```powershell
# 使用 WinDbg 或 Visual Studio 调试器
# 或者使用 dotnet 调试工具
dotnet --fx-version 8.0.0 FirstEngineEditor.dll
```

## 常见调试场景

### 1. 编辑器启动失败

**症状**：编辑器无法启动或立即崩溃

**调试步骤**：
1. 检查 DLL 依赖：
   ```powershell
   # 使用 Dependency Walker 或 dumpbin
   dumpbin /dependents Editor\bin\Debug\net8.0-windows\FirstEngineEditor.exe
   ```

2. 检查 DLL 路径：
   - 确保 `FirstEngine_Core.dll` 在输出目录中
   - 检查 `PATH` 环境变量

3. 查看事件日志：
   - Windows 事件查看器 → Windows 日志 → 应用程序
   - 查找 FirstEngineEditor 相关错误

4. 添加异常处理：
   ```csharp
   // 在 App.xaml.cs 中
   protected override void OnStartup(StartupEventArgs e)
   {
       try
       {
           base.OnStartup(e);
           Services.ServiceLocator.Initialize();
       }
       catch (Exception ex)
       {
           MessageBox.Show($"启动失败: {ex.Message}\n{ex.StackTrace}", "错误");
           Shutdown();
       }
   }
   ```

### 2. 渲染窗口不显示

**症状**：编辑器启动但渲染视口是黑屏

**调试步骤**：
1. 检查 `RenderEngineService.cs` 中的初始化：
   ```csharp
   // 在 RenderEngineService.cs 中添加日志
   public bool Initialize(IntPtr windowHandle, int width, int height)
   {
       try
       {
           _engineHandle = EditorAPI_CreateEngine(windowHandle, width, height);
           if (_engineHandle == IntPtr.Zero)
           {
               System.Diagnostics.Debug.WriteLine("Failed to create engine");
               return false;
           }
           
           if (!EditorAPI_InitializeEngine(_engineHandle))
           {
               System.Diagnostics.Debug.WriteLine("Failed to initialize engine");
               return false;
           }
           
           System.Diagnostics.Debug.WriteLine("Engine initialized successfully");
           return true;
       }
       catch (Exception ex)
       {
           System.Diagnostics.Debug.WriteLine($"Exception: {ex.Message}");
           return false;
       }
   }
   ```

2. 检查 C++ 端日志：
   - 在 `EditorAPI.cpp` 中添加 `std::cout` 输出
   - 查看 Visual Studio 输出窗口

3. 使用 RenderDoc：
   - 启动编辑器
   - 在 RenderDoc 中查看是否有帧被捕获
   - 检查 Vulkan 设备初始化

### 3. P/Invoke 调用失败

**症状**：调用 C++ 函数时出现异常或返回错误

**调试步骤**：
1. 检查函数签名：
   ```csharp
   // 确保 P/Invoke 声明与 C++ 函数匹配
   [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
   private static extern IntPtr EditorAPI_CreateEngine(IntPtr windowHandle, int width, int height);
   ```

2. 检查 DLL 导出：
   ```powershell
   # 查看 DLL 导出的函数
   dumpbin /exports build\bin\Debug\FirstEngine_Core.dll
   ```

3. 使用 `Marshal.GetLastWin32Error()`：
   ```csharp
   IntPtr result = EditorAPI_CreateEngine(handle, width, height);
   if (result == IntPtr.Zero)
   {
       int error = Marshal.GetLastWin32Error();
       System.Diagnostics.Debug.WriteLine($"P/Invoke error: {error}");
   }
   ```

### 4. 内存泄漏或崩溃

**症状**：编辑器运行一段时间后崩溃

**调试步骤**：
1. 使用 Application Verifier：
   - 下载并安装 Application Verifier
   - 添加 FirstEngineEditor.exe
   - 启用基本检查

2. 使用 Visual Studio 诊断工具：
   - 调试 → 性能探查器
   - 选择"内存使用率"
   - 运行应用程序并分析内存使用

3. 检查 C++ 端资源管理：
   - 确保所有 `new` 都有对应的 `delete`
   - 检查智能指针使用
   - 使用 Valgrind（Linux）或 Dr. Memory（Windows）

## 调试工具推荐

### Visual Studio 内置工具
- **断点**：F9 设置/取消断点
- **监视窗口**：调试 → 窗口 → 监视
- **调用堆栈**：调试 → 窗口 → 调用堆栈
- **即时窗口**：Ctrl+Alt+I
- **输出窗口**：查看调试输出

### 外部工具
- **RenderDoc**：图形调试器（已集成）
- **Dependency Walker**：检查 DLL 依赖
- **Process Monitor**：监控文件/注册表访问
- **WinDbg**：高级调试器
- **Application Verifier**：检测常见错误

## 调试技巧

### 1. 使用条件断点
```csharp
// 只在特定条件下中断
if (someCondition)  // 设置断点，右键 → 条件
{
    // 代码
}
```

### 2. 使用日志点
```csharp
// 在断点处输出信息而不中断
// 右键断点 → 操作 → 记录消息
```

### 3. 使用数据断点
- 在 C++ 代码中，可以设置数据断点
- 当变量值改变时中断

### 4. 使用异常断点
- 调试 → 窗口 → 异常设置
- 勾选要中断的异常类型

## 常见问题排查

### DLL 加载失败
```powershell
# 检查 DLL 是否存在
Test-Path "Editor\bin\Debug\net8.0-windows\FirstEngine_Core.dll"

# 检查依赖 DLL
dumpbin /dependents "build\bin\Debug\FirstEngine_Core.dll"
```

### 路径问题
```csharp
// 在代码中输出当前工作目录
System.Diagnostics.Debug.WriteLine($"Current Directory: {Environment.CurrentDirectory}");
System.Diagnostics.Debug.WriteLine($"DLL Path: {System.Reflection.Assembly.GetExecutingAssembly().Location}");
```

### 权限问题
- 确保以管理员权限运行（如果需要）
- 检查文件权限

## 快速调试清单

- [ ] 确保 `FirstEngine_Core.dll` 在输出目录
- [ ] 启用"启用本机代码调试"
- [ ] 设置 C++ 项目调试路径
- [ ] 检查 DLL 依赖是否完整
- [ ] 添加异常处理代码
- [ ] 使用日志输出调试信息
- [ ] 使用 RenderDoc 检查渲染状态

## 示例：完整的调试会话

1. **设置断点**：
   - `App.xaml.cs` 的 `OnStartup` 方法
   - `RenderEngineService.cs` 的 `Initialize` 方法
   - `EditorAPI.cpp` 的 `EditorAPI_CreateEngine` 函数

2. **启动调试**：
   - 按 F5
   - 程序会在第一个断点处停止

3. **单步执行**：
   - F10：逐过程
   - F11：逐语句
   - F5：继续执行

4. **检查变量**：
   - 将鼠标悬停在变量上查看值
   - 在监视窗口中添加表达式

5. **查看调用堆栈**：
   - 了解函数调用关系
   - 可以跳转到调用者

## 联系和支持

如果遇到问题，请检查：
- 日志文件
- Visual Studio 输出窗口
- Windows 事件查看器
- RenderDoc 捕获的帧
