# FrameGraph 详细流程图：从 BuildFrameGraph 到 CommandBuffer

## 完整数据流图

```
┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 1: BuildFrameGraph() - 构造 Node 和 Resource                        │
└─────────────────────────────────────────────────────────────────────────┘

DeferredRenderPipeline::BuildFrameGraph()
    │
    ├─ AddGBufferResources()
    │   │
    │   └─ frameGraph.AddResource("GBufferAlbedo", ResourceDescription)
    │       │
    │       └─ FrameGraph::AddResource()
    │           │
    │           ├─ 创建 FrameGraphResource 对象
    │           │   └─ m_Description = { type: Attachment, width: 1280, height: 720, ... }
    │           │
    │           └─ 存储到 m_Resources["GBufferAlbedo"]
    │               ⚠️ 此时只有描述信息，没有实际的 GPU 资源
    │
    └─ AddGeometryPass()
        │
        └─ frameGraph.AddNode("GeometryPass")
            │
            └─ FrameGraph::AddNode()
                │
                ├─ 创建 FrameGraphNode 对象
                │   ├─ m_Name = "GeometryPass"
                │   ├─ m_Index = 0
                │   ├─ m_Type = "geometry"
                │   ├─ m_ReadResources = []
                │   ├─ m_WriteResources = ["GBufferAlbedo", "GBufferNormal", "GBufferDepth"]
                │   └─ m_ExecuteCallback = Lambda 函数（未执行）
                │
                └─ 存储到 m_Nodes[0]
                    ⚠️ ExecuteCallback 已设置但未执行


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 2: BuildExecutionPlan() - 分析依赖和执行顺序                         │
└─────────────────────────────────────────────────────────────────────────┘

FrameGraph::BuildExecutionPlan()
    │
    ├─ AnalyzeDependencies()
    │   │
    │   ├─ 分析资源生命周期
    │   │   └─ GBufferAlbedo: firstPass=0, lastPass=1
    │   │
    │   └─ 建立节点依赖关系
    │       └─ LightingPass 依赖 GeometryPass（因为读取 GBufferAlbedo）
    │
    ├─ 构建 NodePlan
    │   └─ NodePlan {
    │       name: "GeometryPass",
    │       type: "geometry",
    │       dependencies: []
    │   }
    │
    ├─ 构建 ResourcePlan
    │   └─ ResourcePlan {
    │       name: "GBufferAlbedo",
    │       type: Attachment,
    │       description: { width: 1280, height: 720, ... }
    │   }
    │
    └─ TopologicalSort()
        └─ ExecutionOrder = [0, 1, 2]  // GeometryPass, LightingPass, PostProcessPass


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 3: Compile() - 分配 GPU 资源                                        │
└─────────────────────────────────────────────────────────────────────────┘

FrameGraph::Compile() → AllocateResources()
    │
    └─ 遍历 m_Resources
        │
        └─ 对于每个 ResourceDescription
            │
            ├─ 创建 ImageDescription
            │   └─ { width: 1280, height: 720, format: R8G8B8A8_UNORM, ... }
            │
            ├─ m_Device->CreateImage(imageDesc)
            │   └─ 返回 unique_ptr<IImage>
            │
            └─ resource->SetRHIResource(image.release())
                └─ FrameGraphResource::m_RHIImage = IImage* (实际的 GPU 资源)
                    ✅ 此时资源已分配


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 4: Execute() - 生成 RenderCommandList                               │
└─────────────────────────────────────────────────────────────────────────┘

OnRender()
    │
    ├─ SceneRenderer::Render() → RenderQueue
    │   └─ RenderItem[] { pipeline, vertexBuffer, indexBuffer, ... }
    │
    ├─ SceneRenderer::SubmitRenderQueue() → RenderCommandList
    │   └─ RenderCommand[] {
    │       { type: BindPipeline, params: { pipeline } },
    │       { type: BindVertexBuffers, params: { buffers, offsets } },
    │       { type: DrawIndexed, params: { indexCount: 36, ... } }
    │   }
    │
    └─ FrameGraph::Execute(plan, &sceneCommands)
        │
        ├─ FrameGraphBuilder builder(this)
        │   └─ 用于在 ExecuteCallback 中访问资源
        │
        └─ 按 ExecutionOrder 遍历节点: [0, 1, 2]
            │
            ├─ Node[0]: GeometryPass
            │   │
            │   └─ node->GetExecuteCallback()(builder, sceneCommands)
            │       │
            │       └─ Lambda 执行:
            │           │
            │           ├─ 访问资源: builder.WriteTexture("GBufferAlbedo")
            │           │   └─ 返回 IImage* (已分配的资源)
            │           │
            │           ├─ 生成 BeginRenderPass 命令（TODO）
            │           │
            │           ├─ 合并场景命令
            │           │   └─ for (cmd in sceneCommands) {
            │           │       cmdList.AddCommand(cmd);
            │           │   }
            │           │
            │           └─ 返回 RenderCommandList {
            │               RenderCommand[] {
            │                   { type: BeginRenderPass, ... },
            │                   { type: BindPipeline, ... },
            │                   { type: BindVertexBuffers, ... },
            │                   { type: DrawIndexed, ... },
            │                   { type: EndRenderPass, ... }
            │               }
            │           }
            │
            ├─ Node[1]: LightingPass
            │   │
            │   └─ node->GetExecuteCallback()(builder, sceneCommands)
            │       └─ 返回 RenderCommandList {
            │           RenderCommand[] {
            │               { type: BeginRenderPass, ... },
            │               { type: BindPipeline, ... },  // 光照计算管线
            │               { type: Draw, ... },          // 全屏四边形
            │               { type: EndRenderPass, ... }
            │           }
            │       }
            │
            └─ Node[2]: PostProcessPass
                │
                └─ node->GetExecuteCallback()(builder, sceneCommands)
                    └─ 返回 RenderCommandList {
                        RenderCommand[] {
                            { type: BeginRenderPass, ... },
                            { type: BindPipeline, ... },  // 后处理管线
                            { type: Draw, ... },
                            { type: EndRenderPass, ... }
                        }
                    }
        
        └─ 合并所有节点的命令
            └─ 返回最终的 RenderCommandList {
                RenderCommand[] {
                    // Geometry Pass 命令
                    { type: BeginRenderPass, ... },
                    { type: BindPipeline, ... },
                    { type: DrawIndexed, ... },
                    { type: EndRenderPass, ... },
                    
                    // Lighting Pass 命令
                    { type: BeginRenderPass, ... },
                    { type: BindPipeline, ... },
                    { type: Draw, ... },
                    { type: EndRenderPass, ... },
                    
                    // PostProcess Pass 命令
                    { type: BeginRenderPass, ... },
                    { type: BindPipeline, ... },
                    { type: Draw, ... },
                    { type: EndRenderPass, ... }
                }
            }
            ⚠️ 仍然是数据，不是 GPU 命令


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 5: CommandRecorder - 转换为 GPU 命令                               │
└─────────────────────────────────────────────────────────────────────────┘

OnSubmit()
    │
    ├─ m_CommandBuffer->Begin()
    │
    └─ CommandRecorder::RecordCommands(commandBuffer, renderCommandList)
        │
        └─ 遍历 RenderCommandList
            │
            ├─ RenderCommand { type: BeginRenderPass, ... }
            │   └─ RecordBeginRenderPass()
            │       └─ commandBuffer->BeginRenderPass(
            │           renderPass,
            │           framebuffer,
            │           clearColors,
            │           clearDepth,
            │           clearStencil
            │       )
            │       ✅ GPU 命令已记录
            │
            ├─ RenderCommand { type: BindPipeline, params: { pipeline } }
            │   └─ RecordBindPipeline()
            │       └─ commandBuffer->BindPipeline(pipeline)
            │       ✅ GPU 命令已记录
            │
            ├─ RenderCommand { type: BindVertexBuffers, ... }
            │   └─ RecordBindVertexBuffers()
            │       └─ commandBuffer->BindVertexBuffers(firstBinding, buffers, offsets)
            │       ✅ GPU 命令已记录
            │
            ├─ RenderCommand { type: DrawIndexed, ... }
            │   └─ RecordDrawIndexed()
            │       └─ commandBuffer->DrawIndexed(indexCount, instanceCount, ...)
            │       ✅ GPU 命令已记录
            │
            └─ RenderCommand { type: EndRenderPass, ... }
                └─ RecordEndRenderPass()
                    └─ commandBuffer->EndRenderPass()
                    ✅ GPU 命令已记录


┌─────────────────────────────────────────────────────────────────────────┐
│ 阶段 6: SubmitCommandBuffer() - 提交到 GPU                               │
└─────────────────────────────────────────────────────────────────────────┘

OnSubmit()
    │
    ├─ m_CommandBuffer->End()
    │   └─ 结束命令记录
    │
    ├─ m_Device->SubmitCommandBuffer(...)
    │   │
    │   ├─ 等待: ImageAvailableSemaphore (Swapchain 图像可用)
    │   │
    │   ├─ 提交到 GPU 队列
    │   │   └─ GPU 开始执行命令
    │   │
    │   └─ 信号: RenderFinishedSemaphore (渲染完成)
    │
    └─ m_Swapchain->Present(...)
        └─ 呈现到屏幕
            ✅ 渲染完成
```

## 数据转换详细图

```
BuildFrameGraph() 阶段:
┌─────────────────────────────────────┐
│ FrameGraphNode                       │
│ ├─ m_Name: "GeometryPass"           │
│ ├─ m_Type: "geometry"               │
│ ├─ m_WriteResources: [...]          │
│ └─ m_ExecuteCallback: Lambda        │  ← 函数对象，未执行
└─────────────────────────────────────┘
              │
              │ (设置回调)
              ▼
┌─────────────────────────────────────┐
│ ExecuteCallback                      │
│ std::function<                       │
│   RenderCommandList(                 │
│     FrameGraphBuilder&,              │
│     const RenderCommandList*         │
│   )                                  │
│ >                                    │  ← Lambda 函数体
└─────────────────────────────────────┘


Execute() 阶段:
┌─────────────────────────────────────┐
│ ExecuteCallback 被调用               │
│   builder: FrameGraphBuilder         │
│   sceneCommands: RenderCommandList*  │
└─────────────────────────────────────┘
              │
              │ (执行 Lambda)
              ▼
┌─────────────────────────────────────┐
│ RenderCommandList (返回)              │
│ └─ RenderCommand[] {                │
│     { type: BeginRenderPass, ... },  │
│     { type: BindPipeline, ... },    │
│     { type: DrawIndexed, ... },     │
│     { type: EndRenderPass, ... }    │
│   }                                  │  ← 数据对象
└─────────────────────────────────────┘


CommandRecorder 阶段:
┌─────────────────────────────────────┐
│ RenderCommand                       │
│ ├─ type: BindPipeline               │
│ └─ params: { pipeline: IPipeline* }│  ← 数据
└─────────────────────────────────────┘
              │
              │ (RecordCommand)
              ▼
┌─────────────────────────────────────┐
│ ICommandBuffer                       │
│ └─ BindPipeline(IPipeline*)         │  ← GPU 命令
└─────────────────────────────────────┘
              │
              │ (SubmitCommandBuffer)
              ▼
┌─────────────────────────────────────┐
│ GPU 队列                             │
│ └─ GPU 执行命令                      │  ← GPU 执行
└─────────────────────────────────────┘
```

## ExecuteCallback 的详细执行流程

```
FrameGraph::Execute()
    │
    ├─ 创建 FrameGraphBuilder
    │   └─ builder.m_Graph = this (FrameGraph*)
    │
    └─ for (nodeIndex in executionOrder) {
        │
        ├─ 获取节点
        │   └─ node = m_Nodes[nodeIndex]
        │
        ├─ 检查 ExecuteCallback 是否存在
        │   └─ if (node->GetExecuteCallback())
        │
        └─ 调用 ExecuteCallback
            │
            └─ node->GetExecuteCallback()(builder, sceneCommands)
                │
                ├─ Lambda 函数开始执行
                │   │
                │   ├─ 可以通过 builder 访问资源
                │   │   └─ RHI::IImage* img = builder.WriteTexture("GBufferAlbedo");
                │   │       └─ builder.m_Graph->GetResource("GBufferAlbedo")
                │   │           └─ 返回 FrameGraphResource*
                │   │               └─ resource->GetRHIImage()
                │   │                   └─ 返回 IImage* (已分配的资源)
                │   │
                │   ├─ 可以访问 sceneCommands
                │   │   └─ 包含场景渲染命令（BindPipeline, DrawIndexed 等）
                │   │
                │   ├─ 生成 Pass 特定的命令
                │   │   └─ BeginRenderPass, EndRenderPass 等
                │   │
                │   └─ 返回 RenderCommandList
                │       └─ 包含该 Pass 的所有命令
                │
                └─ 返回的 RenderCommandList 被合并到主命令列表
    }
```

## 关键时间点对比

| 阶段 | Node 状态 | Resource 状态 | ExecuteCallback 状态 | 命令状态 |
|------|----------|--------------|---------------------|---------|
| **BuildFrameGraph()** | 已创建（结构） | 已创建（描述） | 已设置（未执行） | 无 |
| **BuildExecutionPlan()** | 依赖已分析 | 生命周期已分析 | 未执行 | 无 |
| **Compile()** | 不变 | **已分配 GPU 资源** | 未执行 | 无 |
| **Execute()** | 不变 | 不变 | **执行，生成命令** | **RenderCommandList（数据）** |
| **CommandRecorder** | 不变 | 不变 | 不再使用 | **ICommandBuffer（GPU命令）** |
| **SubmitCommandBuffer()** | 不变 | 不变 | 不再使用 | **GPU 队列（执行中）** |

## 示例：Geometry Pass 的完整数据流

### 1. BuildFrameGraph() 时

```cpp
// 设置 ExecuteCallback（不执行）
node->SetExecuteCallback([](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
    // 这个 Lambda 函数对象被存储，但不执行
    // 存储在 FrameGraphNode::m_ExecuteCallback 中
});
```

### 2. Execute() 时

```cpp
// 调用 ExecuteCallback
RenderCommandList nodeCommands = node->GetExecuteCallback()(builder, sceneCommands);

// Lambda 函数现在执行:
// 1. builder.WriteTexture("GBufferAlbedo") 
//    → 返回 IImage* (资源已分配)
// 2. 访问 sceneCommands
//    → 包含场景渲染命令
// 3. 生成命令
RenderCommand beginCmd;
beginCmd.type = RenderCommandType::BeginRenderPass;
cmdList.AddCommand(beginCmd);

// 4. 合并场景命令
for (const auto& cmd : sceneCommands->GetCommands()) {
    cmdList.AddCommand(cmd);
}

// 5. 返回 RenderCommandList
return cmdList;
```

### 3. CommandRecorder 时

```cpp
// 遍历 RenderCommandList
for (const auto& command : commands) {
    // command 是 RenderCommand 对象（数据）
    switch (command.type) {
        case RenderCommandType::BeginRenderPass:
            // 转换为实际的 GPU 命令
            commandBuffer->BeginRenderPass(
                command.params.beginRenderPass.renderPass,
                command.params.beginRenderPass.framebuffer,
                ...
            );
            break;
        // ...
    }
}
```

---

## 总结

**BuildFrameGraph() 到 CommandBuffer 的完整流程**:

1. **BuildFrameGraph()**: 构造 Node 和 Resource 的**结构**，设置 ExecuteCallback（不执行）
2. **BuildExecutionPlan()**: 分析依赖，确定执行顺序
3. **Compile()**: 分配实际的 GPU 资源
4. **Execute()**: **执行** ExecuteCallback，生成 RenderCommandList（数据）
5. **CommandRecorder**: 将 RenderCommandList 转换为 ICommandBuffer API 调用（GPU命令）
6. **SubmitCommandBuffer()**: 提交到 GPU 队列执行

关键点：
- **ExecuteCallback 延迟执行**：在 BuildFrameGraph() 时设置，在 Execute() 时执行
- **数据与命令分离**：Execute() 生成数据，CommandRecorder 转换为命令
- **资源访问时机**：Execute() 时资源已分配，可以安全访问
