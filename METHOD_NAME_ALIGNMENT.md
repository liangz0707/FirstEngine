# 方法名对齐 - RenderApp 和 EditorAPI

## 概述

本次重构对齐了 `RenderApp` 和 `EditorAPI` 的方法名，使它们使用一致的命名风格，便于理解和维护。

## 方法对应关系

### RenderApp (C++ 类方法)

| RenderApp 方法 | 功能 | EditorAPI 对应方法 |
|---------------|------|-------------------|
| `OnPrepareFrameGraph()` | 构建 FrameGraph | `EditorAPI_PrepareFrameGraph()` |
| `OnCreateResources()` | 处理资源 | `EditorAPI_CreateResources()` |
| `OnRender()` | 执行 FrameGraph 生成渲染命令 | `EditorAPI_Render()` |
| `OnSubmit()` | 提交渲染 | `EditorAPI_SubmitFrame()` |

### EditorAPI (C 接口函数)

| EditorAPI 方法 | 功能 | RenderApp 对应方法 |
|---------------|------|-------------------|
| `EditorAPI_PrepareFrameGraph()` | 构建 FrameGraph | `OnPrepareFrameGraph()` |
| `EditorAPI_CreateResources()` | 处理资源 | `OnCreateResources()` |
| `EditorAPI_Render()` | 执行 FrameGraph 生成渲染命令 | `OnRender()` |
| `EditorAPI_SubmitFrame()` | 提交渲染 | `OnSubmit()` |

## 向后兼容性

为了保持向后兼容，保留了旧的方法名：

### 已废弃的方法（保留用于向后兼容）

- `EditorAPI_BeginFrame()` - 内部调用 `EditorAPI_PrepareFrameGraph()` + `EditorAPI_CreateResources()` + `EditorAPI_Render()`
- `EditorAPI_EndFrame()` - 已废弃，不再需要
- `EditorAPI_RenderViewport()` - 内部调用 `EditorAPI_SubmitFrame()`

### C# 服务层

`RenderEngineService` 类也提供了新旧两套方法：

**新方法（推荐使用）**：
```csharp
public void PrepareFrameGraph()
public void CreateResources()
public void Render()
public void SubmitFrame()
```

**旧方法（已废弃，保留用于向后兼容）**：
```csharp
public void BeginFrame()
public void EndFrame()
public void RenderViewport()
```

## 使用示例

### RenderApp 流程

```cpp
// 在 Application::MainLoop() 中自动调用
OnPrepareFrameGraph();  // 构建 FrameGraph
OnCreateResources();     // 处理资源
OnRender();              // 执行 FrameGraph
OnSubmit();              // 提交渲染
```

### EditorAPI 流程

```cpp
// 在 C# 的渲染循环中调用
EditorAPI_PrepareFrameGraph(engine);  // 构建 FrameGraph
EditorAPI_CreateResources(engine);     // 处理资源
EditorAPI_Render(engine);              // 执行 FrameGraph
EditorAPI_SubmitFrame(viewport);       // 提交渲染
```

### C# 使用示例

```csharp
// 在 RenderView.xaml.cs 的渲染循环中
_renderEngine.PrepareFrameGraph();
_renderEngine.CreateResources();
_renderEngine.Render();
_renderEngine.SubmitFrame();
```

## 优势

1. **命名一致性**：RenderApp 和 EditorAPI 使用相同的命名风格
2. **易于理解**：方法名清晰表达了功能
3. **便于维护**：两个路径的方法名对应，便于同步修改
4. **向后兼容**：保留了旧方法，不会破坏现有代码

## 迁移指南

### 从旧方法迁移到新方法

**C++ 代码**：
```cpp
// 旧方式
EditorAPI_BeginFrame(engine);
EditorAPI_RenderViewport(viewport);

// 新方式
EditorAPI_PrepareFrameGraph(engine);
EditorAPI_CreateResources(engine);
EditorAPI_Render(engine);
EditorAPI_SubmitFrame(viewport);
```

**C# 代码**：
```csharp
// 旧方式
_renderEngine.BeginFrame();
_renderEngine.RenderViewport();
_renderEngine.EndFrame();

// 新方式
_renderEngine.PrepareFrameGraph();
_renderEngine.CreateResources();
_renderEngine.Render();
_renderEngine.SubmitFrame();
```

## 文件变更

### 修改的文件

- `include/FirstEngine/Editor/EditorAPI.h` - 添加新方法声明，标记旧方法为已废弃
- `src/Editor/EditorAPI.cpp` - 实现新方法，旧方法内部调用新方法
- `Editor/Services/RenderEngineService.cs` - 添加新方法的 P/Invoke 声明和包装
- `Editor/Views/Panels/RenderView.xaml.cs` - 更新渲染循环使用新方法
