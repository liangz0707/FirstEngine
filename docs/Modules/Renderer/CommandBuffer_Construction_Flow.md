# CommandBuffer 构造流程详解

本文档详细说明从 FrameGraph 构造、Scene 数据收集到 CommandBuffer 提交的完整流程。

## 整体架构概览

```
初始化阶段 (Initialize)
    ↓
每帧循环 (MainLoop)
    ├─ OnUpdate()          - 游戏逻辑更新
    ├─ OnPrepareFrameGraph() - 构建渲染管线执行计划
    ├─ OnRender()          - 场景遍历和数据收集
    └─ OnSubmit()          - CommandBuffer 记录和提交
```

---

## 阶段一：初始化 (Initialize)

### 1.1 FrameGraph 构建

**位置**: `RenderApp::Initialize()`

```cpp
// 1. 创建 PipelineConfig（从文件或默认配置）
PipelineConfig config = PipelineBuilder::CreateDeferredRenderingConfig(width, height);

// 2. 使用 PipelineBuilder 构建 FrameGraph
PipelineBuilder builder(m_Device);
m_FrameGraph = new FrameGraph(m_Device);
builder.BuildFromConfig(config, *m_FrameGraph);
```

**PipelineBuilder::BuildFromConfig() 做了什么**:
- 添加资源（G-Buffer、深度缓冲等）
- 添加节点（Passes：Geometry、Lighting、Forward、Post-process）
- 为每个节点设置执行回调（ExecuteCallback）
  - Geometry Pass: 合并场景渲染命令
  - Lighting Pass: 光照计算
  - Forward Pass: 前向渲染
  - Post-process Pass: 后处理

**此时的状态**:
- FrameGraph 结构已建立（节点和资源）
- 执行回调已设置，但**尚未执行**
- 资源**尚未分配**

---

## 阶段二：OnPrepareFrameGraph() - 构建执行计划

**调用时机**: 每帧，在 `OnRender()` 之前

### 2.1 构建执行计划 (BuildExecutionPlan)

```cpp
m_FrameGraph->BuildExecutionPlan(m_FrameGraphPlan);
```

**流程**:
1. **分析依赖关系** (`AnalyzeDependencies()`)
   - 分析资源的使用生命周期（firstPass, lastPass）
   - 建立节点之间的依赖关系

2. **构建节点计划** (NodePlan)
   ```cpp
   NodePlan {
       name: "GeometryPass"
       index: 0
       type: "geometry"
       readResources: []
       writeResources: ["GBufferAlbedo", "GBufferNormal", "GBufferDepth"]
       dependencies: []
   }
   ```

3. **构建资源计划** (ResourcePlan)
   ```cpp
   ResourcePlan {
       name: "GBufferAlbedo"
       type: ResourceType::Attachment
       description: { width, height, format, ... }
   }
   ```

4. **拓扑排序** (`TopologicalSort()`)
   - 确定节点的执行顺序
   - 例如: [GeometryPass, LightingPass, PostProcessPass]

**输出**: `FrameGraphExecutionPlan`
- 与 CommandBuffer 和 Device **无关**的中间数据结构
- 描述渲染管线的结构和资源需求

### 2.2 编译 FrameGraph (Compile)

```cpp
m_FrameGraph->Compile(m_FrameGraphPlan);
```

**流程**:
1. **分配资源** (`AllocateResources()`)
   - 根据 ResourcePlan 创建实际的 RHI 资源
   - 创建 Image、Buffer 等 GPU 资源
   - 设置资源的 RHI 句柄

**此时的状态**:
- 执行计划已构建
- GPU 资源已分配
- **尚未生成任何渲染命令**

---

## 阶段三：OnRender() - 场景遍历和数据收集

**调用时机**: 每帧，在 `OnPrepareFrameGraph()` 之后

### 3.1 场景遍历 (Scene Traversal)

```cpp
m_SceneRenderer->Render(m_RenderQueue, m_RenderConfig);
```

**流程**:
1. **视锥剔除** (Frustum Culling)
   - 使用 `RenderConfig` 获取 ViewMatrix 和 ProjectionMatrix
   - 构建视锥体
   - 查询场景中的可见实体

2. **构建渲染队列** (`BuildRenderQueue()`)
   - 遍历可见实体
   - 为每个实体创建 `RenderItem`
     ```cpp
     RenderItem {
         pipeline: IPipeline*
         descriptorSet: void*
         vertexBuffer: IBuffer*
         indexBuffer: IBuffer*
         indexCount: uint32_t
         ...
     }
     ```
   - 按 Pipeline 和 Material 分组到 `RenderBatch`

**输出**: `RenderQueue`
- 包含多个 `RenderBatch`
- 每个 `RenderBatch` 包含多个 `RenderItem`
- **纯数据结构，无 GPU 命令**

### 3.2 转换为渲染命令列表

```cpp
m_SceneRenderCommands = m_SceneRenderer->SubmitRenderQueue(m_RenderQueue);
```

**流程** (`SceneRenderer::SubmitRenderQueue()`):
1. 遍历 `RenderQueue` 中的每个 `RenderBatch`
2. 为每个 `RenderItem` 生成 `RenderCommand`:
   ```cpp
   // BindPipeline
   RenderCommand { type: BindPipeline, params: { pipeline } }
   
   // BindDescriptorSets
   RenderCommand { type: BindDescriptorSets, params: { descriptorSets } }
   
   // BindVertexBuffers
   RenderCommand { type: BindVertexBuffers, params: { buffers, offsets } }
   
   // BindIndexBuffer
   RenderCommand { type: BindIndexBuffer, params: { buffer, offset } }
   
   // DrawIndexed
   RenderCommand { type: DrawIndexed, params: { indexCount, ... } }
   ```

**输出**: `RenderCommandList` (m_SceneRenderCommands)
- 包含场景渲染命令的数据结构
- **仍然是数据，不是 GPU 命令**

### 3.3 FrameGraph 执行

```cpp
m_FrameGraphRenderCommands = m_FrameGraph->Execute(m_FrameGraphPlan, &m_SceneRenderCommands);
```

**流程** (`FrameGraph::Execute()`):
1. 按照执行计划的顺序遍历节点
2. 对每个节点调用其 `ExecuteCallback`:
   ```cpp
   // Geometry Pass 回调示例
   [](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
       RenderCommandList cmdList;
       
       // 1. BeginRenderPass (TODO)
       // 2. 合并场景命令
       if (sceneCommands) {
           for (const auto& cmd : sceneCommands->GetCommands()) {
               cmdList.AddCommand(cmd);
           }
       }
       // 3. EndRenderPass (TODO)
       
       return cmdList;
   }
   ```
3. 合并所有节点的命令列表

**输出**: `RenderCommandList` (m_FrameGraphRenderCommands)
- 包含完整的渲染管线命令（包括合并的场景命令）
- **仍然是数据，不是 GPU 命令**

**此时的状态**:
- 场景数据已收集
- 渲染命令已生成（数据形式）
- **尚未创建 CommandBuffer**
- **尚未记录任何 GPU 命令**

---

## 阶段四：OnSubmit() - CommandBuffer 记录和提交

**调用时机**: 每帧，在 `OnRender()` 之后

### 4.1 等待上一帧完成

```cpp
m_Device->WaitForFence(m_InFlightFence, UINT64_MAX);
m_Device->ResetFence(m_InFlightFence);
m_CommandBuffer.reset(); // 释放上一帧的 CommandBuffer
```

### 4.2 获取 Swapchain 图像

```cpp
m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore, nullptr, m_CurrentImageIndex);
```

### 4.3 创建并开始记录 CommandBuffer

```cpp
m_CommandBuffer = m_Device->CreateCommandBuffer();
m_CommandBuffer->Begin();
```

### 4.4 资源布局转换

```cpp
// Swapchain 图像: Undefined → Color Attachment
m_CommandBuffer->TransitionImageLayout(
    swapchainImage,
    Format::Undefined,
    Format::B8G8R8A8_UNORM,
    1
);
```

### 4.5 记录渲染命令

```cpp
m_CommandRecorder.RecordCommands(m_CommandBuffer.get(), m_FrameGraphRenderCommands);
```

**流程** (`CommandRecorder::RecordCommands()`):
1. 遍历 `RenderCommandList` 中的每个 `RenderCommand`
2. 根据命令类型调用相应的记录方法:

```cpp
switch (command.type) {
    case RenderCommandType::BindPipeline:
        commandBuffer->BindPipeline(params.pipeline);
        break;
    case RenderCommandType::BindDescriptorSets:
        commandBuffer->BindDescriptorSets(...);
        break;
    case RenderCommandType::BindVertexBuffers:
        commandBuffer->BindVertexBuffers(...);
        break;
    case RenderCommandType::BindIndexBuffer:
        commandBuffer->BindIndexBuffer(...);
        break;
    case RenderCommandType::DrawIndexed:
        commandBuffer->DrawIndexed(...);
        break;
    case RenderCommandType::BeginRenderPass:
        commandBuffer->BeginRenderPass(...);
        break;
    case RenderCommandType::EndRenderPass:
        commandBuffer->EndRenderPass();
        break;
    // ... 其他命令类型
}
```

**此时**: 命令已记录到 CommandBuffer（GPU 命令）

### 4.6 资源布局转换（后处理）

```cpp
// Swapchain 图像: Color Attachment → Present
m_CommandBuffer->TransitionImageLayout(
    swapchainImage,
    Format::B8G8R8A8_UNORM,
    Format::B8G8R8A8_SRGB,
    1
);
```

### 4.7 结束记录

```cpp
m_CommandBuffer->End();
```

### 4.8 提交 CommandBuffer

```cpp
std::vector<SemaphoreHandle> waitSemaphores = {m_ImageAvailableSemaphore};
std::vector<SemaphoreHandle> signalSemaphores = {m_RenderFinishedSemaphore};
m_Device->SubmitCommandBuffer(
    m_CommandBuffer.get(),
    waitSemaphores,
    signalSemaphores,
    m_InFlightFence
);
```

**同步机制**:
- **等待**: `ImageAvailableSemaphore` - 等待 Swapchain 图像可用
- **信号**: `RenderFinishedSemaphore` - 渲染完成后信号
- **Fence**: `InFlightFence` - 等待 GPU 完成执行

### 4.9 呈现到屏幕

```cpp
std::vector<SemaphoreHandle> presentWaitSemaphores = {m_RenderFinishedSemaphore};
m_Swapchain->Present(m_CurrentImageIndex, presentWaitSemaphores);
```

---

## 数据流图

```
初始化阶段:
PipelineConfig → PipelineBuilder → FrameGraph (结构)

每帧循环:

OnPrepareFrameGraph():
    FrameGraph → BuildExecutionPlan() → FrameGraphExecutionPlan
    FrameGraph → Compile() → 分配 GPU 资源

OnRender():
    Scene + RenderConfig → SceneRenderer::Render() → RenderQueue
    RenderQueue → SceneRenderer::SubmitRenderQueue() → RenderCommandList (场景命令)
    FrameGraphExecutionPlan + SceneCommands → FrameGraph::Execute() → RenderCommandList (完整命令)

OnSubmit():
    RenderCommandList → CommandRecorder::RecordCommands() → ICommandBuffer (GPU 命令)
    ICommandBuffer → Device::SubmitCommandBuffer() → GPU 队列
    GPU 队列 → Swapchain::Present() → 屏幕
```

---

## 关键设计点

### 1. 数据与命令分离

- **OnRender()**: 生成数据（RenderCommandList）
- **OnSubmit()**: 将数据转换为 GPU 命令

**优势**:
- 多线程友好（数据生成可以并行）
- 逻辑隔离（渲染决策与 GPU API 调用分离）

### 2. FrameGraph 与场景命令合并

- 场景命令在 `FrameGraph::Execute()` 中合并到 Geometry/Forward Pass
- 统一的命令列表，正确的执行顺序

### 3. 执行计划 (ExecutionPlan)

- 在 `OnPrepareFrameGraph()` 中构建
- 与 CommandBuffer/Device 无关
- 可以用于决定需要收集哪些场景资源

### 4. 资源生命周期管理

- 资源在 `Compile()` 时分配
- 根据执行计划确定资源的使用范围
- 自动处理资源重用和释放

---

## 命令类型列表

| 命令类型 | 说明 | 使用位置 |
|---------|------|---------|
| `BindPipeline` | 绑定渲染管线 | SceneRenderer, FrameGraph |
| `BindDescriptorSets` | 绑定描述符集 | SceneRenderer |
| `BindVertexBuffers` | 绑定顶点缓冲区 | SceneRenderer |
| `BindIndexBuffer` | 绑定索引缓冲区 | SceneRenderer |
| `Draw` | 绘制（无索引） | SceneRenderer |
| `DrawIndexed` | 索引绘制 | SceneRenderer |
| `BeginRenderPass` | 开始渲染通道 | FrameGraph Passes |
| `EndRenderPass` | 结束渲染通道 | FrameGraph Passes |
| `TransitionImageLayout` | 图像布局转换 | OnSubmit (Swapchain) |
| `PushConstants` | 推送常量 | SceneRenderer (TODO) |

---

## 总结

整个流程实现了**完全的逻辑隔离**:

1. **决策阶段** (`OnPrepareFrameGraph`, `OnRender`): 
   - 构建渲染管线结构
   - 收集场景数据
   - 生成渲染命令（数据形式）

2. **执行阶段** (`OnSubmit`):
   - 将数据转换为 GPU 命令
   - 提交到 GPU
   - 呈现到屏幕

这种设计使得系统具有高度的**灵活性**和**可扩展性**，便于后续的多线程优化和渲染管线扩展。
