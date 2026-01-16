# CommandBuffer 构造流程图

## 完整流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        初始化阶段 (Initialize)                            │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  PipelineConfig (配置文件)          │
        │  - 资源定义 (G-Buffer, Depth)       │
        │  - Pass 定义 (Geometry, Lighting)   │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  PipelineBuilder::BuildFromConfig() │
        │  - 添加资源到 FrameGraph            │
        │  - 添加节点 (Passes)                │
        │  - 设置 ExecuteCallback             │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  FrameGraph (结构已建立)            │
        │  - Nodes: [Geometry, Lighting, ...] │
        │  - Resources: [GBuffer, ...]        │
        │  - ExecuteCallbacks: 已设置         │
        │  ⚠️ 资源尚未分配                    │
        └─────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│                    每帧循环 (MainLoop)                                    │
└─────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 1: OnPrepareFrameGraph()                                            │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  FrameGraph::BuildExecutionPlan()    │
        │  - AnalyzeDependencies()            │
        │  - 构建 NodePlan                     │
        │  - 构建 ResourcePlan                │
        │  - TopologicalSort()                 │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  FrameGraphExecutionPlan            │
        │  - NodePlans: [执行顺序]            │
        │  - ResourcePlans: [资源需求]        │
        │  - ExecutionOrder: [0,1,2,...]      │
        │  ⚠️ 与 CommandBuffer 无关           │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  FrameGraph::Compile()               │
        │  - AllocateResources()               │
        │  - 创建 RHI Image/Buffer             │
        │  - 设置资源句柄                     │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  GPU 资源已分配                      │
        │  - GBufferAlbedo (Image)            │
        │  - GBufferNormal (Image)            │
        │  - GBufferDepth (Image)             │
        └─────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 2: OnRender() - 场景遍历和数据收集                                    │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  SceneRenderer::Render()             │
        │  - 使用 RenderConfig 获取矩阵        │
        │  - 视锥剔除 (Frustum Culling)       │
        │  - 遍历可见实体                      │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  RenderQueue                         │
        │  - RenderBatch[]                    │
        │    └─ RenderItem[]                   │
        │       ├─ pipeline                   │
        │       ├─ vertexBuffer               │
        │       ├─ indexBuffer                │
        │       └─ indexCount                 │
        │  ⚠️ 纯数据结构，无 GPU 命令         │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  SceneRenderer::SubmitRenderQueue()  │
        │  - 遍历 RenderQueue                  │
        │  - 为每个 RenderItem 生成命令        │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  RenderCommandList (场景命令)        │
        │  - RenderCommand[]                  │
        │    ├─ BindPipeline                  │
        │    ├─ BindVertexBuffers             │
        │    ├─ BindIndexBuffer               │
        │    └─ DrawIndexed                   │
        │  ⚠️ 仍然是数据，不是 GPU 命令       │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  FrameGraph::Execute()               │
        │  - 按执行顺序遍历节点                │
        │  - 调用每个节点的 ExecuteCallback   │
        │  - 传递场景命令给 Geometry Pass     │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  Geometry Pass Callback              │
        │  - BeginRenderPass (TODO)            │
        │  - 合并场景命令                      │
        │  - EndRenderPass (TODO)             │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  RenderCommandList (完整命令)        │
        │  - Geometry Pass 命令               │
        │  - Lighting Pass 命令               │
        │  - Post-process Pass 命令           │
        │  - 包含合并的场景命令                │
        │  ⚠️ 仍然是数据，不是 GPU 命令       │
        └─────────────────────────────────────┘


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 3: OnSubmit() - CommandBuffer 记录和提交                            │
└─────────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  等待上一帧完成                       │
        │  - WaitForFence()                    │
        │  - ResetFence()                      │
        │  - 释放上一帧的 CommandBuffer        │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  获取 Swapchain 图像                 │
        │  - AcquireNextImage()                │
        │  - 等待 ImageAvailableSemaphore     │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  创建 CommandBuffer                  │
        │  - CreateCommandBuffer()             │
        │  - Begin()                           │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  资源布局转换 (Swapchain)            │
        │  - TransitionImageLayout()           │
        │  - Undefined → Color Attachment      │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  CommandRecorder::RecordCommands()  │
        │  - 遍历 RenderCommandList            │
        │  - 根据类型调用相应方法              │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  ICommandBuffer (GPU 命令)          │
        │  - BindPipeline()                    │
        │  - BindVertexBuffers()              │
        │  - DrawIndexed()                     │
        │  - BeginRenderPass()                 │
        │  - EndRenderPass()                   │
        │  ✅ 真正的 GPU 命令                 │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  资源布局转换 (后处理)               │
        │  - TransitionImageLayout()           │
        │  - Color Attachment → Present        │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  结束记录                            │
        │  - CommandBuffer::End()              │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  提交到 GPU                          │
        │  - SubmitCommandBuffer()             │
        │  - 等待: ImageAvailableSemaphore    │
        │  - 信号: RenderFinishedSemaphore     │
        │  - Fence: InFlightFence             │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  呈现到屏幕                          │
        │  - Swapchain::Present()              │
        │  - 等待: RenderFinishedSemaphore    │
        └─────────────────────────────────────┘
                              │
                              ▼
        ┌─────────────────────────────────────┐
        │  屏幕显示                            │
        │  ✅ 渲染完成                         │
        └─────────────────────────────────────┘
```

## 数据转换链

```
Scene (场景数据)
    ↓
RenderConfig (全局配置)
    ↓
RenderQueue (渲染队列)
    ├─ RenderBatch[]
    │   └─ RenderItem[]
    │       ├─ pipeline
    │       ├─ vertexBuffer
    │       ├─ indexBuffer
    │       └─ indexCount
    ↓
RenderCommandList (场景命令 - 数据)
    └─ RenderCommand[]
        ├─ BindPipeline
        ├─ BindVertexBuffers
        ├─ BindIndexBuffer
        └─ DrawIndexed
    ↓
FrameGraph::Execute() (合并到 Pass)
    ↓
RenderCommandList (完整命令 - 数据)
    └─ RenderCommand[]
        ├─ BeginRenderPass
        ├─ BindPipeline (场景)
        ├─ DrawIndexed (场景)
        ├─ EndRenderPass
        └─ ... (其他 Pass 命令)
    ↓
CommandRecorder::RecordCommands()
    ↓
ICommandBuffer (GPU 命令)
    ├─ BindPipeline()
    ├─ BindVertexBuffers()
    ├─ DrawIndexed()
    ├─ BeginRenderPass()
    └─ EndRenderPass()
    ↓
GPU Queue (GPU 队列)
    ↓
屏幕显示
```

## 关键数据结构

### RenderQueue
```cpp
RenderQueue
├─ RenderBatch[] (按 Pipeline 分组)
│   └─ RenderItem[]
│       ├─ pipeline: IPipeline*
│       ├─ descriptorSet: void*
│       ├─ vertexBuffer: IBuffer*
│       ├─ indexBuffer: IBuffer*
│       └─ indexCount: uint32_t
```

### RenderCommandList
```cpp
RenderCommandList
└─ RenderCommand[]
    ├─ type: RenderCommandType
    └─ params: (union-like struct)
        ├─ bindPipeline: { pipeline }
        ├─ bindVertexBuffers: { buffers[], offsets[] }
        ├─ bindIndexBuffer: { buffer, offset }
        └─ drawIndexed: { indexCount, ... }
```

### FrameGraphExecutionPlan
```cpp
FrameGraphExecutionPlan
├─ NodePlan[]
│   ├─ name: string
│   ├─ type: string ("geometry", "lighting", ...)
│   ├─ readResources: string[]
│   ├─ writeResources: string[]
│   └─ dependencies: uint32_t[]
├─ ResourcePlan[]
│   ├─ name: string
│   ├─ type: ResourceType
│   └─ description: ResourceDescription
└─ ExecutionOrder: uint32_t[]
```

## 时间线

```
Frame N:
    OnPrepareFrameGraph() ──────┐
    OnRender() ──────────────────┤ (CPU 工作)
    OnSubmit() ──────────────────┘
                                  │
                                  ▼
    GPU 执行 ─────────────────────┐
                                  │
                                  ▼
    Present ──────────────────────┘

Frame N+1:
    OnPrepareFrameGraph() ──────┐
    OnRender() ──────────────────┤ (CPU 工作，与 GPU 并行)
    OnSubmit() ──────────────────┘
                                  │
                                  ▼
    GPU 执行 ─────────────────────┐
                                  │
                                  ▼
    Present ──────────────────────┘
```

## 多线程优化潜力

当前架构支持以下多线程优化：

1. **OnRender() 阶段**:
   - 场景遍历可以并行（多个线程处理不同的实体子集）
   - RenderQueue 构建可以并行
   - RenderCommandList 生成可以并行

2. **OnPrepareFrameGraph() 阶段**:
   - 执行计划构建可以并行（节点计划、资源计划）
   - 资源分配可以并行（不同资源类型）

3. **OnSubmit() 阶段**:
   - 必须单线程（CommandBuffer 记录）
   - 但可以在不同的 CommandBuffer 上并行记录（多帧并行）
