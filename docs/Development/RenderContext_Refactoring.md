# 渲染逻辑重构 - RenderContext 中间层

## 概述

本次重构将 `RenderApp` 和 `EditorAPI` 中重复的渲染逻辑提取到一个统一的中间层 `RenderContext` 中，消除了代码重复，提高了可维护性。

## 重构内容

### 1. 创建 RenderContext 类

**位置**：
- 头文件：`include/FirstEngine/Renderer/RenderContext.h`
- 实现文件：`src/Renderer/RenderContext.cpp`

**功能**：
- 封装 FrameGraph 构建逻辑（`BeginFrame`）
- 封装 FrameGraph 执行逻辑（`ExecuteFrameGraph`）
- 封装渲染提交逻辑（`SubmitFrame`）
- 管理同步对象（可选，支持外部提供或内部管理）

### 2. 重构 RenderApp

**修改的方法**：
- `OnPrepareFrameGraph()` - 使用 `RenderContext::BeginFrame()`
- `OnRender()` - 使用 `RenderContext::ExecuteFrameGraph()`
- `OnSubmit()` - 使用 `RenderContext::SubmitFrame()`

**代码减少**：
- `OnPrepareFrameGraph()`: 从 ~40 行减少到 ~15 行
- `OnSubmit()`: 从 ~95 行减少到 ~20 行

### 3. 重构 EditorAPI

**修改的函数**：
- `EditorAPI_BeginFrame()` - 使用 `RenderContext::BeginFrame()` 和 `ExecuteFrameGraph()`
- `EditorAPI_RenderViewport()` - 使用 `RenderContext::SubmitFrame()`

**代码减少**：
- `EditorAPI_BeginFrame()`: 从 ~30 行减少到 ~25 行
- `EditorAPI_RenderViewport()`: 从 ~70 行减少到 ~20 行

## 架构设计

### RenderContext::RenderParams

统一的参数结构，包含渲染所需的所有参数：

```cpp
struct RenderParams {
    RHI::IDevice* device = nullptr;
    IRenderPipeline* renderPipeline = nullptr;
    FrameGraph* frameGraph = nullptr;
    Scene* scene = nullptr;
    RenderConfig* renderConfig = nullptr;
    RHI::ISwapchain* swapchain = nullptr;
    
    // 同步对象（可选）
    RHI::FenceHandle inFlightFence = nullptr;
    RHI::SemaphoreHandle imageAvailableSemaphore = nullptr;
    RHI::SemaphoreHandle renderFinishedSemaphore = nullptr;
    
    // 当前图像索引（输出参数）
    uint32_t* currentImageIndex = nullptr;
    
    // 命令缓冲区和记录器（可选）
    std::unique_ptr<RHI::ICommandBuffer>* commandBuffer = nullptr;
    CommandRecorder* commandRecorder = nullptr;
};
```

### 使用流程

#### RenderApp 流程

```cpp
// 1. OnPrepareFrameGraph() - 构建 FrameGraph
RenderContext::RenderParams params;
params.renderPipeline = m_RenderPipeline;
params.frameGraph = m_FrameGraph;
params.renderConfig = &m_RenderConfig;
m_RenderContext.BeginFrame(params);

// 2. OnRender() - 执行 FrameGraph
params.scene = m_Scene;
m_RenderContext.ExecuteFrameGraph(params);

// 3. OnSubmit() - 提交渲染
params.device = m_Device;
params.swapchain = m_Swapchain.get();
params.inFlightFence = m_InFlightFence;
// ... 其他同步对象
m_RenderContext.SubmitFrame(params);
```

#### EditorAPI 流程

```cpp
// 1. EditorAPI_BeginFrame() - 构建和执行 FrameGraph
RenderContext::RenderParams params;
params.renderPipeline = engine->renderPipeline;
params.frameGraph = engine->frameGraph;
params.renderConfig = &engine->renderConfig;
engine->renderContext.BeginFrame(params);

params.scene = engine->scene;
engine->renderContext.ExecuteFrameGraph(params);
engine->renderContext.ProcessResources(engine->device, 0);

// 2. EditorAPI_RenderViewport() - 提交渲染
params.device = engine->device;
params.swapchain = viewport->swapchain;
// ... 其他参数
engine->renderContext.SubmitFrame(params);
```

## 优势

1. **代码复用**：消除了 RenderApp 和 EditorAPI 之间的重复代码
2. **易于维护**：渲染逻辑集中在一个地方，修改更容易
3. **一致性**：确保两个路径使用相同的渲染逻辑
4. **灵活性**：支持外部提供同步对象和命令缓冲区，或使用内部管理
5. **可测试性**：RenderContext 可以独立测试

## 兼容性

- RenderApp 和 EditorAPI 的公共接口保持不变
- 内部实现使用 RenderContext，对外部调用者透明
- 保留了原有的成员变量（如 `m_FrameGraphRenderCommands`）以保持兼容性

## 后续改进

1. **进一步简化**：可以考虑移除 RenderApp 和 EditorAPI 中不再需要的成员变量
2. **错误处理**：可以增强 RenderContext 的错误处理和日志记录
3. **性能优化**：可以在 RenderContext 中添加性能统计和优化

## 文件变更

### 新增文件
- `include/FirstEngine/Renderer/RenderContext.h`
- `src/Renderer/RenderContext.cpp`

### 修改文件
- `src/Application/RenderApp.cpp`
- `src/Editor/EditorAPI.cpp`
- `src/Renderer/CMakeLists.txt`（添加 RenderContext.cpp）
