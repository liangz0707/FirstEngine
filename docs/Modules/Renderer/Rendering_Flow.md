# 渲染流程文档

本文档描述了 RenderApp 和 EditorAPI 的渲染执行流程，确保两者的一致性。

## 执行流程对比

### RenderApp 流程（通过 Application::MainLoop）

```
1. OnUpdate(deltaTime)
   └─> 更新逻辑（游戏状态等）

2. OnPrepareFrameGraph()
   └─> RenderDocHelper::BeginFrame()          // 开始 RenderDoc 帧捕获
   └─> RenderContext::BeginFrame()            // 构建 FrameGraph 执行计划

3. OnRender()
   └─> 更新 RenderConfig（如果窗口尺寸改变）
   └─> RenderContext::ExecuteFrameGraph()     // 执行 FrameGraph，生成渲染命令

4. OnCreateResources()
   └─> RenderContext::ProcessResources()      // 处理计划中的资源

5. OnSubmit()
   └─> RenderContext::SubmitFrame()           // 提交命令缓冲区，呈现图像
   └─> RenderDocHelper::EndFrame()             // 结束 RenderDoc 帧捕获
```

### EditorAPI 流程（通过 C# CompositionTarget.Rendering）

```
1. EditorAPI_PrepareFrameGraph(engine)
   └─> RenderDocHelper::BeginFrame()          // 开始 RenderDoc 帧捕获
   └─> RenderContext::BeginFrame()            // 构建 FrameGraph 执行计划

2. EditorAPI_CreateResources(engine)
   └─> RenderContext::ProcessResources()     // 处理计划中的资源

3. EditorAPI_Render(engine)
   └─> RenderContext::ExecuteFrameGraph()     // 执行 FrameGraph，生成渲染命令

4. EditorAPI_SubmitFrame(viewport)
   └─> RenderContext::SubmitFrame()          // 提交命令缓冲区，呈现图像
   └─> RenderDocHelper::EndFrame()            // 结束 RenderDoc 帧捕获
```

## 关键差异说明

1. **RenderConfig 更新**：
   - RenderApp：在 `OnRender()` 中每帧检查并更新 RenderConfig（如果窗口尺寸改变）
   - EditorAPI：通过 `EditorAPI_SetRenderConfig()` 或 `EditorAPI_ResizeViewport()` 显式更新
   - 原因：EditorAPI 的窗口尺寸由 C# 端管理，不需要每帧检查

2. **资源处理顺序**：
   - RenderApp：`OnCreateResources()` 在 `OnRender()` 之后
   - EditorAPI：`EditorAPI_CreateResources()` 在 `EditorAPI_Render()` 之前
   - 注意：这个顺序差异是合理的，因为资源处理可以在渲染之前或之后进行

## 废弃的 API

以下函数已废弃，但为了向后兼容保留：

- `EditorAPI_BeginFrame()` - 内部调用 `PrepareFrameGraph + CreateResources + Render`
- `EditorAPI_EndFrame()` - 空函数，不再需要
- `EditorAPI_RenderViewport()` - 内部调用 `EditorAPI_SubmitFrame()`

## RenderDoc 集成

RenderDoc 支持通过共享的 `RenderDocHelper` 类提供：

- **初始化**：在创建 Vulkan 实例之前调用 `RenderDocHelper::Initialize()`
- **帧捕获**：在帧开始时调用 `RenderDocHelper::BeginFrame()`，在帧提交后调用 `RenderDocHelper::EndFrame()`

## 确保流程完整性

### RenderApp
- ✅ OnPrepareFrameGraph - 调用 BeginFrame 和 RenderDocHelper::BeginFrame
- ✅ OnRender - 更新 RenderConfig 和 ExecuteFrameGraph
- ✅ OnCreateResources - 调用 ProcessResources
- ✅ OnSubmit - 调用 SubmitFrame 和 RenderDocHelper::EndFrame

### EditorAPI
- ✅ EditorAPI_PrepareFrameGraph - 调用 BeginFrame 和 RenderDocHelper::BeginFrame
- ✅ EditorAPI_CreateResources - 调用 ProcessResources
- ✅ EditorAPI_Render - 调用 ExecuteFrameGraph
- ✅ EditorAPI_SubmitFrame - 调用 SubmitFrame 和 RenderDocHelper::EndFrame

## 代码组织

- **共享代码**：RenderDoc 集成代码已提取到 `FirstEngine::Core::RenderDocHelper`
- **统一接口**：RenderApp 和 EditorAPI 都通过 `RenderContext` 进行渲染操作
- **一致性**：两个路径使用相同的 RenderContext 方法，确保行为一致
