# FirstEngine Editor

FirstEngine 游戏引擎的 C# WPF 编辑器。

## 功能特性

- ✅ **文件管理**: 项目创建、打开、保存
- ✅ **Scene 列表**: 查看和管理场景文件
- ✅ **资源列表**: 浏览和管理引擎资源（Model、Material、Texture等）
- ✅ **属性面板**: 编辑选中对象的属性
- ✅ **渲染窗口**: 3D 场景预览（待实现与引擎集成）

## 项目结构

```
Editor/
├── ViewModels/          # MVVM ViewModels
│   ├── MainViewModel.cs
│   └── Panels/         # 各个面板的 ViewModel
├── Views/              # WPF Views
│   ├── MainWindow.xaml
│   └── Panels/         # 各个面板的 View
├── Services/           # 服务层
│   ├── IProjectManager.cs
│   ├── ProjectManager.cs
│   ├── IFileManager.cs
│   └── FileManager.cs
├── Models/             # 数据模型
└── Styles/             # WPF 样式
```

## 技术栈

- **.NET 8.0**: 目标框架
- **WPF**: UI 框架
- **MVVM**: 架构模式（使用 Microsoft.Toolkit.Mvvm）
- **Newtonsoft.Json**: JSON 序列化

## 构建和运行

### 前置要求

- .NET 8.0 SDK
- Visual Studio 2022 或更高版本（推荐）

### 构建

```bash
cd Editor
dotnet build
```

### 运行

```bash
dotnet run
```

或在 Visual Studio 中按 F5 运行。

## 使用说明

### 创建新项目

1. 点击 `File` -> `New Project`
2. 选择项目文件夹
3. 输入项目名称
4. 编辑器会自动创建项目结构

### 打开项目

1. 点击 `File` -> `Open Project`
2. 选择包含 `project.json` 的项目文件夹

### 查看场景

- 左侧面板显示所有场景文件
- 双击场景可加载（待实现）

### 查看资源

- 左侧下方显示资源列表
- 从 `resource_manifest.json` 加载资源信息
- 选择资源可在属性面板查看详情

### 编辑属性

- 右侧属性面板显示选中对象的属性
- 支持编辑属性值（待完善）

## 待实现功能

- [ ] 与 FirstEngine 引擎的集成（渲染窗口）
- [ ] 场景编辑器（实体创建、变换编辑）
- [ ] 资源导入工具集成
- [ ] 材质编辑器
- [ ] 着色器编辑器
- [ ] 动画编辑器
- [ ] 完整的属性编辑支持

## 开发计划

1. **Phase 1**: 基础框架 ✅
   - MVVM 架构
   - 文件管理
   - 基本 UI 布局

2. **Phase 2**: 核心功能（进行中）
   - Scene 加载和显示
   - 资源管理
   - 属性编辑

3. **Phase 3**: 引擎集成
   - 渲染窗口集成
   - 实时预览
   - 场景编辑

4. **Phase 4**: 高级功能
   - 材质编辑器
   - 着色器编辑器
   - 动画系统

## 贡献

欢迎提交 Issue 和 Pull Request！
