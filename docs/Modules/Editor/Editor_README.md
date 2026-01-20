# FirstEngine Editor

FirstEngine 游戏引擎的编辑器模块，使用 C# WPF 实现，参考 UE5 的界面布局。

## 功能特性

- **项目管理**: 创建、打开、保存项目
- **场景编辑**: 场景列表、资源列表、属性面板
- **渲染视口**: 集成 FirstEngine 渲染引擎，实时预览场景
- **资源管理**: 导入和管理游戏资源

## 架构

### C# 编辑器层
- **Views**: WPF 界面视图
  - `MainWindow.xaml`: 主窗口，包含菜单、工具栏、面板布局
  - `RenderView.xaml`: 渲染视口面板
  - `SceneListView.xaml`: 场景列表面板
  - `ResourceListView.xaml`: 资源列表面板
  - `PropertyPanelView.xaml`: 属性面板

- **ViewModels**: MVVM 模式的数据绑定
  - `MainViewModel`: 主窗口视图模型
  - `RenderViewModel`: 渲染视口视图模型
  - `SceneListViewModel`: 场景列表视图模型
  - `ResourceListViewModel`: 资源列表视图模型
  - `PropertyPanelViewModel`: 属性面板视图模型

- **Services**: 业务逻辑服务
  - `RenderEngineService`: 渲染引擎服务（P/Invoke 包装）
  - `ProjectManager`: 项目管理服务
  - `FileManager`: 文件管理服务
  - `ServiceLocator`: 服务定位器

### C++ 引擎层
- **EditorAPI**: C 接口导出，供 C# 调用
  - `EditorAPI.h`: C 接口声明
  - `EditorAPI.cpp`: C 接口实现

## 启动模式

编辑器支持两种启动模式：

### 1. 编辑器模式 (默认)
```bash
FirstEngine.exe --editor
# 或
FirstEngine.exe -e
```
启动 C# WPF 编辑器界面。

### 2. 独立渲染窗口模式
```bash
FirstEngine.exe --standalone
# 或
FirstEngine.exe -s
```
直接启动渲染窗口，不显示编辑器界面。

## 构建

### 前置要求
- .NET 8.0 SDK
- Visual Studio 2022 (推荐)
- CMake 3.20+

### 构建步骤

1. **构建 C++ 引擎**
```powershell
.\build_project.ps1
```

2. **构建 C# 编辑器**
```powershell
cd Editor
dotnet build
```

3. **运行编辑器**
```powershell
# 从引擎目录运行
.\bin\Debug\FirstEngine.exe --editor
```

## 项目结构

```
Editor/
├── App.xaml                 # 应用程序入口
├── App.xaml.cs
├── FirstEngineEditor.csproj # 项目文件
├── Models/                  # 数据模型
│   └── Project.cs
├── Services/                # 服务层
│   ├── RenderEngineService.cs
│   ├── ProjectManager.cs
│   ├── FileManager.cs
│   └── ServiceLocator.cs
├── ViewModels/              # 视图模型
│   ├── ViewModelBase.cs
│   └── Panels/
│       ├── RenderViewModel.cs
│       ├── SceneListViewModel.cs
│       ├── ResourceListViewModel.cs
│       └── PropertyPanelViewModel.cs
├── Views/                   # 视图
│   ├── MainWindow.xaml
│   ├── MainWindow.xaml.cs
│   ├── InputDialog.xaml
│   └── Panels/
│       ├── RenderView.xaml
│       ├── RenderView.xaml.cs
│       ├── SceneListView.xaml
│       ├── ResourceListView.xaml
│       └── PropertyPanelView.xaml
└── Styles/                  # 样式资源
    └── CommonStyles.xaml
```

## 开发说明

### 添加新功能

1. **添加新面板**:
   - 在 `Views/Panels/` 创建 XAML 视图
   - 在 `ViewModels/Panels/` 创建对应的 ViewModel
   - 在 `MainWindow.xaml` 中添加面板引用

2. **扩展渲染引擎接口**:
   - 在 `include/FirstEngine/Editor/EditorAPI.h` 添加 C 接口声明
   - 在 `src/Editor/EditorAPI.cpp` 实现接口
   - 在 `Editor/Services/RenderEngineService.cs` 添加 P/Invoke 声明

3. **添加新服务**:
   - 创建服务类实现接口
   - 在 `ServiceLocator.Initialize()` 中注册服务

## 注意事项

- 编辑器需要 FirstEngine_Core.dll 等 C++ DLL 在可执行文件目录
- 渲染视口使用 P/Invoke 调用 C++ 引擎
- 确保 Vulkan SDK 已正确安装和配置
