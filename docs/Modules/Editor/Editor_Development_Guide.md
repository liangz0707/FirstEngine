# FirstEngine 编辑器开发指南

## 架构概述

FirstEngine 编辑器采用 **C# WPF + C++ 引擎** 的混合架构：

```
┌─────────────────────────────────────────────────────────┐
│              C# WPF 编辑器层 (Editor/)                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐            │
│  │  Views   │  │ ViewModels│  │ Services │            │
│  │  (XAML)  │  │  (MVVM)   │  │ (业务逻辑)│            │
│  └──────────┘  └──────────┘  └──────────┘            │
│         │              │              │               │
│         └──────────────┼──────────────┘               │
│                        │                                │
│              ┌─────────▼─────────┐                     │
│              │ RenderEngineService│                    │
│              │   (P/Invoke)      │                     │
│              └─────────┬─────────┘                     │
└─────────────────────────┼───────────────────────────────┘
                          │
                          │ DLL 调用
                          ▼
┌─────────────────────────────────────────────────────────┐
│         C++ 引擎层 (src/Editor/EditorAPI.cpp)          │
│  ┌──────────────────────────────────────────────┐     │
│  │  EditorAPI.h  (C 接口声明)                    │     │
│  │  EditorAPI.cpp (C 接口实现)                   │     │
│  └──────────────────────────────────────────────┘     │
│                        │                                │
│              ┌─────────▼─────────┐                     │
│              │  FirstEngine 引擎  │                    │
│              │  (Vulkan, Render)  │                     │
│              └────────────────────┘                     │
└─────────────────────────────────────────────────────────┘
```

## 项目结构

### C# 编辑器项目 (`Editor/`)

```
Editor/
├── FirstEngineEditor.csproj    # C# 项目文件
├── App.xaml / App.xaml.cs      # 应用程序入口
│
├── Models/                      # 数据模型
│   └── Project.cs               # 项目数据模型
│
├── Services/                    # 业务逻辑服务层
│   ├── RenderEngineService.cs  # 渲染引擎服务 (P/Invoke 包装)
│   ├── ProjectManager.cs        # 项目管理服务
│   ├── FileManager.cs          # 文件管理服务
│   └── ServiceLocator.cs       # 服务定位器 (依赖注入)
│
├── ViewModels/                  # MVVM 视图模型
│   ├── ViewModelBase.cs        # ViewModel 基类
│   ├── MainViewModel.cs        # 主窗口 ViewModel
│   └── Panels/                 # 面板 ViewModels
│       ├── RenderViewModel.cs
│       ├── SceneListViewModel.cs
│       ├── ResourceListViewModel.cs
│       └── PropertyPanelViewModel.cs
│
├── Views/                       # WPF 视图 (XAML)
│   ├── MainWindow.xaml         # 主窗口
│   ├── MainWindow.xaml.cs      # 主窗口代码后置
│   └── Panels/                 # 面板视图
│       ├── RenderView.xaml
│       ├── RenderView.xaml.cs
│       ├── SceneListView.xaml
│       ├── ResourceListView.xaml
│       └── PropertyPanelView.xaml
│
├── Controls/                    # 自定义控件
│   └── RenderViewportHost.cs   # 视口嵌入控件 (HwndHost)
│
└── Styles/                      # 样式资源
    └── CommonStyles.xaml
```

### C++ 引擎接口 (`src/Editor/`)

```
src/Editor/
└── EditorAPI.cpp               # C 接口实现

include/FirstEngine/Editor/
└── EditorAPI.h                 # C 接口声明
```

## 开发流程

### 1. 添加新的编辑器功能

#### 场景 1: 添加新的 UI 面板

**步骤：**

1. **创建 ViewModel** (`Editor/ViewModels/Panels/MyPanelViewModel.cs`)
   ```csharp
   using FirstEngineEditor.ViewModels;
   
   namespace FirstEngineEditor.ViewModels.Panels
   {
       public class MyPanelViewModel : ViewModelBase
       {
           private string _title = "My Panel";
           
           public string Title
           {
               get => _title;
               set => SetProperty(ref _title, value);
           }
       }
   }
   ```

2. **创建 View** (`Editor/Views/Panels/MyPanelView.xaml`)
   ```xml
   <UserControl x:Class="FirstEngineEditor.Views.Panels.MyPanelView"
                xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
       <Grid>
           <TextBlock Text="{Binding Title}"/>
       </Grid>
   </UserControl>
   ```

3. **创建代码后置** (`Editor/Views/Panels/MyPanelView.xaml.cs`)
   ```csharp
   using System.Windows.Controls;
   using FirstEngineEditor.ViewModels.Panels;
   
   namespace FirstEngineEditor.Views.Panels
   {
       public partial class MyPanelView : UserControl
       {
           public MyPanelView()
           {
               InitializeComponent();
               DataContext = new MyPanelViewModel();
           }
       }
   }
   ```

4. **在主窗口中添加面板** (`Editor/Views/MainWindow.xaml`)
   ```xml
   <panels:MyPanelView Grid.Column="2"/>
   ```

#### 场景 2: 扩展 C++ 引擎接口

**步骤：**

1. **在 C++ 头文件中声明接口** (`include/FirstEngine/Editor/EditorAPI.h`)
   ```cpp
   // 添加新接口声明
   FE_CORE_API bool EditorAPI_LoadTexture(RenderEngine* engine, const char* texturePath);
   FE_CORE_API void EditorAPI_UnloadTexture(RenderEngine* engine, const char* texturePath);
   ```

2. **在 C++ 中实现接口** (`src/Editor/EditorAPI.cpp`)
   ```cpp
   extern "C" {
       
   bool EditorAPI_LoadTexture(RenderEngine* engine, const char* texturePath) {
       if (!engine || !engine->initialized || !texturePath) {
           return false;
       }
       
       try {
           // 调用引擎的纹理加载功能
           // 例如：engine->device->LoadTexture(texturePath);
           return true;
       } catch (...) {
           return false;
       }
   }
   
   void EditorAPI_UnloadTexture(RenderEngine* engine, const char* texturePath) {
       if (!engine || !texturePath) return;
       // 实现卸载逻辑
   }
   
   } // extern "C"
   ```

3. **在 C# 中添加 P/Invoke 声明** (`Editor/Services/RenderEngineService.cs`)
   ```csharp
   [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
   [return: MarshalAs(UnmanagedType.I1)]
   private static extern bool EditorAPI_LoadTexture(IntPtr engine, string texturePath);
   
   [DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
   private static extern void EditorAPI_UnloadTexture(IntPtr engine, string texturePath);
   ```

4. **在 RenderEngineService 中添加包装方法**
   ```csharp
   public bool LoadTexture(string texturePath)
   {
       if (_initialized && _engineHandle != IntPtr.Zero)
       {
           return EditorAPI_LoadTexture(_engineHandle, texturePath);
       }
       return false;
   }
   
   public void UnloadTexture(string texturePath)
   {
       if (_initialized && _engineHandle != IntPtr.Zero)
       {
           EditorAPI_UnloadTexture(_engineHandle, texturePath);
       }
   }
   ```

5. **在 ViewModel 中使用服务**
   ```csharp
   public class ResourceListViewModel : ViewModelBase
   {
       private readonly RenderEngineService _renderEngine;
       
       public void LoadTextureCommand(string path)
       {
           if (_renderEngine.LoadTexture(path))
           {
               // 更新 UI
           }
       }
   }
   ```

#### 场景 3: 添加新的服务

**步骤：**

1. **定义服务接口** (`Editor/Services/IMyService.cs`)
   ```csharp
   namespace FirstEngineEditor.Services
   {
       public interface IMyService
       {
           void DoSomething();
       }
   }
   ```

2. **实现服务** (`Editor/Services/MyService.cs`)
   ```csharp
   namespace FirstEngineEditor.Services
   {
       public class MyService : IMyService
       {
           public void DoSomething()
           {
               // 实现逻辑
           }
       }
   }
   ```

3. **在 ServiceLocator 中注册** (`Editor/Services/ServiceLocator.cs`)
   ```csharp
   public static void Initialize()
   {
       _services = new Dictionary<Type, object>();
       
       // 注册服务
       Register<IProjectManager>(new ProjectManager());
       Register<IFileManager>(new FileManager());
       Register<IMyService>(new MyService());  // 添加新服务
   }
   ```

4. **在 ViewModel 中使用**
   ```csharp
   public class MyViewModel : ViewModelBase
   {
       private readonly IMyService _myService;
       
       public MyViewModel()
       {
           _myService = ServiceLocator.Get<IMyService>();
       }
   }
   ```

### 2. 编译和调试

#### 编译 C++ 引擎

```powershell
# 方法 1: 使用脚本
.\build_project.ps1

# 方法 2: 手动编译
cd build
cmake ..
cmake --build . --config Debug
```

#### 编译 C# 编辑器

```powershell
cd Editor
dotnet build FirstEngineEditor.csproj -c Debug
```

#### 调试流程

1. **调试 C# 编辑器**：
   - 在 Visual Studio 中打开 `Editor/FirstEngineEditor.csproj`
   - 设置断点
   - 按 F5 启动调试

2. **调试 C++ 引擎**：
   - 在 Visual Studio 中打开 `build/FirstEngine.sln`
   - 设置 `FirstEngine_Editor` 为启动项目
   - 设置断点
   - 按 F5 启动调试

3. **混合调试**（同时调试 C# 和 C++）：
   - 在 Visual Studio 中打开解决方案
   - 右键解决方案 → 属性 → 启动项目
   - 选择 "多个启动项目"
   - 设置 `FirstEngineEditor` 和 `FirstEngine_Editor` 都启动

### 3. 常见开发任务

#### 任务 1: 添加菜单项

**文件**: `Editor/Views/MainWindow.xaml`

```xml
<MenuItem Header="_File">
    <MenuItem Header="_New Project" Click="OnNewProject"/>
    <MenuItem Header="My New Menu" Click="OnMyNewMenu"/>  <!-- 添加 -->
</MenuItem>
```

**文件**: `Editor/Views/MainWindow.xaml.cs`

```csharp
private void OnMyNewMenu(object sender, RoutedEventArgs e)
{
    // 实现逻辑
}
```

#### 任务 2: 添加工具栏按钮

**文件**: `Editor/Views/MainWindow.xaml`

```xml
<ToolBar>
    <Button Content="Play" Click="OnPlay"/>
    <Button Content="My Button" Click="OnMyButton"/>  <!-- 添加 -->
</ToolBar>
```

#### 任务 3: 添加属性绑定

**ViewModel**:
```csharp
private bool _isEnabled = true;

public bool IsEnabled
{
    get => _isEnabled;
    set => SetProperty(ref _isEnabled, value);
}
```

**View**:
```xml
<Button IsEnabled="{Binding IsEnabled}" Content="Click Me"/>
```

#### 任务 4: 处理视口输入

**文件**: `Editor/Controls/RenderViewportHost.cs`

```csharp
protected override IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
{
    switch (msg)
    {
        case 0x0201: // WM_LBUTTONDOWN
            // 处理鼠标左键按下
            break;
        case 0x0200: // WM_MOUSEMOVE
            // 处理鼠标移动
            break;
    }
    return base.WndProc(hwnd, msg, wParam, lParam, ref handled);
}
```

## 关键概念

### 1. P/Invoke (Platform Invoke)

C# 通过 P/Invoke 调用 C++ DLL：

```csharp
[DllImport("FirstEngine_Core.dll", CallingConvention = CallingConvention.Cdecl)]
private static extern IntPtr EditorAPI_CreateEngine(IntPtr windowHandle, int width, int height);
```

**注意事项**：
- `CallingConvention.Cdecl`：C 调用约定
- `CharSet.Ansi`：字符串编码
- `MarshalAs(UnmanagedType.I1)`：布尔值编组

### 2. MVVM 模式

- **Model**: 数据模型 (`Models/`)
- **View**: UI 视图 (`Views/`)
- **ViewModel**: 视图模型 (`ViewModels/`)

**数据绑定示例**:
```xml
<!-- View -->
<TextBlock Text="{Binding Title}"/>
```

```csharp
// ViewModel
public string Title { get; set; }
```

### 3. 服务定位器模式

使用 `ServiceLocator` 进行依赖注入：

```csharp
var service = ServiceLocator.Get<IMyService>();
```

### 4. 视口嵌入

使用 `HwndHost` 将 Win32 窗口嵌入到 WPF：

```csharp
public class RenderViewportHost : HwndHost
{
    public IntPtr ViewportHandle { get; set; }
    
    protected override HandleRef BuildWindowCore(HandleRef hwndParent)
    {
        // 创建或嵌入子窗口
        return new HandleRef(this, ViewportHandle);
    }
}
```

## 开发最佳实践

### 1. 代码组织

- **单一职责**：每个类只负责一个功能
- **依赖注入**：使用 `ServiceLocator` 而不是直接实例化
- **MVVM 分离**：View 只负责显示，ViewModel 处理逻辑

### 2. 错误处理

```csharp
try
{
    if (!_renderEngine.Initialize(windowHandle, width, height))
    {
        // 显示错误消息
        MessageBox.Show("Failed to initialize render engine");
    }
}
catch (Exception ex)
{
    System.Diagnostics.Debug.WriteLine($"Error: {ex.Message}");
}
```

### 3. 资源管理

```csharp
public class MyService : IDisposable
{
    public void Dispose()
    {
        // 清理资源
    }
}
```

### 4. 异步操作

```csharp
private async Task LoadSceneAsync(string path)
{
    await Task.Run(() =>
    {
        _renderEngine.LoadScene(path);
    });
}
```

## 调试技巧

### 1. 查看 P/Invoke 调用

在 C++ 中添加日志：
```cpp
std::cerr << "EditorAPI_CreateEngine called" << std::endl;
```

### 2. 检查 DLL 加载

```csharp
// 检查 DLL 是否存在
if (!File.Exists("FirstEngine_Core.dll"))
{
    MessageBox.Show("FirstEngine_Core.dll not found!");
}
```

### 3. 使用 Visual Studio 调试器

- **条件断点**：设置条件 `engine != nullptr`
- **数据断点**：监控变量值变化
- **调用堆栈**：查看 C# → C++ 调用链

## 常见问题

### Q: 编辑器无法找到 FirstEngine_Core.dll

**A**: 确保 DLL 在编辑器可执行文件目录：
```powershell
Copy-Item "build\lib\Debug\FirstEngine_Core.dll" "Editor\bin\Debug\net8.0-windows\"
```

### Q: P/Invoke 调用失败

**A**: 检查：
1. DLL 名称是否正确
2. 函数签名是否匹配
3. 调用约定是否正确

### Q: 视口不显示

**A**: 检查：
1. 视口窗口句柄是否正确
2. 窗口样式是否正确设置
3. 父窗口句柄是否有效

## 下一步

1. **扩展渲染接口**：添加更多 C++ 功能暴露给 C#
2. **实现场景编辑**：添加场景对象选择和编辑
3. **资源导入工具**：实现资源导入对话框
4. **属性编辑器**：实现动态属性编辑
5. **撤销/重做系统**：实现操作历史记录

## 参考资源

- [WPF 文档](https://docs.microsoft.com/en-us/dotnet/desktop/wpf/)
- [P/Invoke 指南](https://www.pinvoke.net/)
- [MVVM 模式](https://docs.microsoft.com/en-us/dotnet/desktop/wpf/get-started/commanding-overview)
