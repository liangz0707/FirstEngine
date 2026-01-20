# FirstEngine 编辑器设置指南

## 概述

FirstEngine 编辑器是一个基于 C# WPF 的编辑器，用于编辑和管理游戏项目。编辑器通过 P/Invoke 调用 C++ 渲染引擎。

## 构建步骤

### 1. 构建 C++ 引擎

```powershell
# 在项目根目录
.\build_project.ps1
```

这将构建所有 C++ 模块，包括 `FirstEngine_Editor` 静态库。

### 2. 构建 C# 编辑器

```powershell
cd Editor
dotnet build -c Release
```

### 3. 复制 DLL 文件

编辑器需要以下 DLL 文件在输出目录：
- `FirstEngine_Core.dll`
- `FirstEngine_Device.dll`
- `FirstEngine_RHI.dll`
- `FirstEngine_Shader.dll`
- `vulkan-1.dll` (如果不在系统目录)

可以手动复制，或使用项目文件中的 `CopyNativeDlls` Target（需要配置 SolutionDir）。

## 运行

### 编辑器模式

```powershell
# 从引擎可执行文件启动编辑器
.\bin\Debug\FirstEngine.exe --editor

# 或直接运行编辑器可执行文件
.\Editor\bin\Release\net8.0-windows\FirstEngineEditor.exe
```

### 独立渲染窗口模式

```powershell
.\bin\Debug\FirstEngine.exe --standalone
```

## 项目结构

```
FirstEngine/
├── Editor/                    # C# 编辑器项目
│   ├── FirstEngineEditor.csproj
│   ├── App.xaml
│   ├── Services/              # 服务层
│   │   ├── RenderEngineService.cs  # P/Invoke 包装
│   │   ├── ProjectManager.cs
│   │   └── ServiceLocator.cs
│   ├── ViewModels/            # MVVM 视图模型
│   ├── Views/                 # WPF 视图
│   └── Models/                # 数据模型
│
├── include/FirstEngine/Editor/
│   └── EditorAPI.h            # C 接口声明
│
└── src/Editor/
    ├── CMakeLists.txt
    └── EditorAPI.cpp          # C 接口实现
```

## 开发说明

### 添加新的编辑器功能

1. **添加新面板**:
   - 在 `Editor/Views/Panels/` 创建 XAML 视图
   - 在 `Editor/ViewModels/Panels/` 创建对应的 ViewModel
   - 在 `MainWindow.xaml` 中引用新面板

2. **扩展渲染引擎接口**:
   - 在 `include/FirstEngine/Editor/EditorAPI.h` 添加 C 函数声明
   - 在 `src/Editor/EditorAPI.cpp` 实现函数
   - 在 `Editor/Services/RenderEngineService.cs` 添加 P/Invoke 声明

3. **添加新服务**:
   - 创建服务类（实现接口或继承基类）
   - 在 `ServiceLocator.Initialize()` 中注册

### P/Invoke 注意事项

- 所有导出函数必须使用 `extern "C"` 和 `__declspec(dllexport)`
- 函数调用约定使用 `CallingConvention.Cdecl`
- 字符串参数使用 `CharSet.Ansi`
- 布尔返回值使用 `[return: MarshalAs(UnmanagedType.I1)]`

## 故障排除

### 编辑器无法启动

1. 检查 DLL 文件是否在输出目录
2. 检查 Vulkan SDK 是否正确安装
3. 查看错误日志（控制台输出）

### 渲染视口不显示

1. 检查 `RenderEngineService` 是否正确初始化
2. 检查窗口句柄是否正确传递
3. 查看 Vulkan 验证层错误（如果启用）

### DLL 加载失败

1. 确保所有依赖 DLL 在同一目录
2. 检查平台目标（x64）是否匹配
3. 使用 Dependency Walker 检查依赖关系

## 下一步开发

- [ ] 实现子窗口/视口嵌入（使用 HwndHost）
- [ ] 添加场景编辑功能
- [ ] 实现资源导入工具
- [ ] 添加属性编辑器
- [ ] 实现撤销/重做系统
