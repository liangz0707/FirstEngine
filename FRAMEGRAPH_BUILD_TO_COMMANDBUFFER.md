# FrameGraph 从 BuildFrameGraph 到 CommandBuffer 的完整流程

本文档详细说明从 `BuildFrameGraph()` 构造 Node 到最终提交给 CommandBuffer 的完整数据流和转换过程。

---

## 整体流程图

```
BuildFrameGraph() (构造阶段)
    ↓
添加 Resources (资源描述)
    ↓
添加 Nodes (节点) + 设置 ExecuteCallback
    ↓
BuildExecutionPlan() (分析阶段)
    ↓
Compile() (资源分配阶段)
    ↓
Execute() (命令生成阶段)
    ↓
CommandRecorder (命令转换阶段)
    ↓
ICommandBuffer (GPU命令)
    ↓
SubmitCommandBuffer() (提交阶段)
```

---

## 阶段一：BuildFrameGraph() - 构造 Node 和 Resource

### 1.1 调用入口

**位置**: `OnPrepareFrameGraph()` → `IRenderPipeline::BuildFrameGraph()`

```cpp
// RenderApp::OnPrepareFrameGraph()
m_RenderPipeline->BuildFrameGraph(*m_FrameGraph, m_RenderConfig);
```

### 1.2 DeferredRenderPipeline::BuildFrameGraph() 流程

```cpp
bool DeferredRenderPipeline::BuildFrameGraph(FrameGraph& frameGraph, const RenderConfig& config) {
    // 1. 清理旧的图结构
    frameGraph.Clear();
    
    // 2. 添加资源（Resource）
    AddGBufferResources(frameGraph, resolution);
    AddOutputResources(frameGraph, resolution);
    
    // 3. 添加节点（Node/Pass）
    if (pipelineConfig.deferredSettings.geometryPassEnabled) {
        AddGeometryPass(frameGraph);
    }
    if (pipelineConfig.deferredSettings.lightingPassEnabled) {
        AddLightingPass(frameGraph);
    }
    if (pipelineConfig.deferredSettings.postProcessPassEnabled) {
        AddPostProcessPass(frameGraph);
    }
    
    return true;
}
```

### 1.3 添加资源（Resource）

**示例**: `AddGBufferResources()`

```cpp
void DeferredRenderPipeline::AddGBufferResources(FrameGraph& frameGraph, const ResolutionConfig& resolution) {
    // 创建资源描述
    ResourceDescription albedoDesc;
    albedoDesc.type = ResourceType::Attachment;
    albedoDesc.name = "GBufferAlbedo";
    albedoDesc.width = resolution.width;
    albedoDesc.height = resolution.height;
    albedoDesc.format = RHI::Format::R8G8B8A8_UNORM;
    
    // 添加到 FrameGraph
    frameGraph.AddResource("GBufferAlbedo", albedoDesc);
}
```

**FrameGraph::AddResource() 做了什么**:
```cpp
FrameGraphResource* FrameGraph::AddResource(const std::string& name, const ResourceDescription& desc) {
    // 创建 FrameGraphResource 对象
    auto resource = std::make_unique<FrameGraphResource>(name, desc);
    FrameGraphResource* resourcePtr = resource.get();
    
    // 存储到 m_Resources map 中
    m_Resources[name] = std::move(resource);
    
    return resourcePtr;
}
```

**此时的状态**:
- `FrameGraphResource` 对象已创建
- 包含资源的**描述信息**（width, height, format等）
- **尚未分配实际的 GPU 资源**（IImage/IBuffer 指针为 nullptr）

### 1.4 添加节点（Node/Pass）

**示例**: `AddGeometryPass()`

```cpp
void DeferredRenderPipeline::AddGeometryPass(FrameGraph& frameGraph) {
    // 1. 创建节点
    auto* node = frameGraph.AddNode("GeometryPass");
    
    // 2. 设置节点类型
    node->SetType("geometry");
    
    // 3. 声明资源访问关系
    node->AddWriteResource("GBufferAlbedo");
    node->AddWriteResource("GBufferNormal");
    node->AddWriteResource("GBufferDepth");
    
    // 4. 设置执行回调（关键！）
    node->SetExecuteCallback([](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
        RenderCommandList cmdList;
        
        // 这个回调会在 Execute() 时被调用
        // 它返回一个 RenderCommandList（数据），不直接操作 CommandBuffer
        
        // 合并场景命令
        if (sceneCommands && !sceneCommands->IsEmpty()) {
            for (const auto& cmd : sceneCommands->GetCommands()) {
                cmdList.AddCommand(cmd);
            }
        }
        
        return cmdList;
    });
}
```

**FrameGraph::AddNode() 做了什么**:
```cpp
FrameGraphNode* FrameGraph::AddNode(const std::string& name) {
    // 创建 FrameGraphNode 对象
    uint32_t index = static_cast<uint32_t>(m_Nodes.size());
    auto node = std::make_unique<FrameGraphNode>(name, index);
    FrameGraphNode* nodePtr = node.get();
    
    // 存储到 m_Nodes vector 中
    m_Nodes.push_back(std::move(node));
    m_NodeNameToIndex[name] = index;
    
    return nodePtr;
}
```

**FrameGraphNode 的结构**:
```cpp
class FrameGraphNode {
    std::string m_Name;                    // "GeometryPass"
    uint32_t m_Index;                      // 0, 1, 2...
    std::string m_Type;                    // "geometry", "lighting", "postprocess"
    std::vector<std::string> m_ReadResources;   // ["GBufferAlbedo", ...]
    std::vector<std::string> m_WriteResources;  // ["GBufferAlbedo", ...]
    std::vector<uint32_t> m_Dependencies;       // [0, 1, ...] 依赖的节点索引
    ExecuteCallback m_ExecuteCallback;     // Lambda 函数
};
```

**此时的状态**:
- `FrameGraphNode` 对象已创建
- 资源访问关系已声明（Read/Write）
- `ExecuteCallback` 已设置（但**尚未执行**）
- **尚未生成任何渲染命令**

---

## 阶段二：BuildExecutionPlan() - 分析依赖和执行顺序

### 2.1 分析依赖关系

**位置**: `FrameGraph::BuildExecutionPlan()`

```cpp
bool FrameGraph::BuildExecutionPlan(FrameGraphExecutionPlan& plan) {
    // 1. 分析依赖关系
    AnalyzeDependencies();
    
    // 2. 构建节点计划
    for (const auto& node : m_Nodes) {
        FrameGraphExecutionPlan::NodePlan nodePlan;
        nodePlan.name = node->GetName();
        nodePlan.type = node->GetType();
        nodePlan.readResources = node->GetReadResources();
        nodePlan.writeResources = node->GetWriteResources();
        nodePlan.dependencies = node->GetDependencies();
        plan.AddNodePlan(std::move(nodePlan));
    }
    
    // 3. 构建资源计划
    for (const auto& [resourceName, resource] : m_Resources) {
        FrameGraphExecutionPlan::ResourcePlan resourcePlan;
        resourcePlan.name = resource->GetName();
        resourcePlan.type = resource->GetDescription().type;
        resourcePlan.description = resource->GetDescription();
        plan.AddResourcePlan(std::move(resourcePlan));
    }
    
    // 4. 拓扑排序，确定执行顺序
    std::vector<uint32_t> executionOrder = TopologicalSort();
    plan.SetExecutionOrder(executionOrder);
    
    return plan.IsValid();
}
```

**AnalyzeDependencies() 做了什么**:
1. 分析每个资源的生命周期（firstPass, lastPass）
2. 建立节点之间的依赖关系
   - 如果 Node B 读取 Node A 写入的资源，则 B 依赖 A
   - 例如: LightingPass 读取 GBufferAlbedo（GeometryPass 写入），所以 LightingPass 依赖 GeometryPass

**TopologicalSort() 做了什么**:
- 使用 Kahn 算法进行拓扑排序
- 确保依赖的节点先执行
- 返回执行顺序，例如: `[0, 1, 2]` (GeometryPass, LightingPass, PostProcessPass)

**输出**: `FrameGraphExecutionPlan`
```cpp
FrameGraphExecutionPlan {
    NodePlans: [
        { name: "GeometryPass", type: "geometry", dependencies: [] },
        { name: "LightingPass", type: "lighting", dependencies: [0] },
        { name: "PostProcessPass", type: "postprocess", dependencies: [1] }
    ],
    ResourcePlans: [
        { name: "GBufferAlbedo", type: Attachment, ... },
        ...
    ],
    ExecutionOrder: [0, 1, 2]  // 执行顺序
}
```

---

## 阶段三：Compile() - 分配 GPU 资源

### 3.1 资源分配

**位置**: `FrameGraph::Compile()` → `AllocateResources()`

```cpp
bool FrameGraph::AllocateResources() {
    for (auto& [resourceName, resource] : m_Resources) {
        const auto& desc = resource->GetDescription();
        
        if (desc.type == ResourceType::Attachment) {
            // 创建 Image
            RHI::ImageDescription imageDesc;
            imageDesc.width = desc.width;
            imageDesc.height = desc.height;
            imageDesc.format = desc.format;
            imageDesc.usage = RHI::ImageUsageFlags::ColorAttachment;
            
            // 调用 Device 创建实际的 GPU 资源
            auto image = m_Device->CreateImage(imageDesc);
            
            // 存储到 FrameGraphResource
            resource->SetRHIResource(image.release());  // 转移所有权
        }
    }
    
    return true;
}
```

**此时的状态**:
- `FrameGraphResource` 现在包含实际的 `IImage*` 或 `IBuffer*` 指针
- GPU 资源已分配
- **仍然没有生成任何渲染命令**

---

## 阶段四：Execute() - 生成 RenderCommandList

### 4.1 调用入口

**位置**: `OnRender()` → `FrameGraph::Execute()`

```cpp
// RenderApp::OnRender()
m_FrameGraphRenderCommands = m_FrameGraph->Execute(m_FrameGraphPlan, &m_SceneRenderCommands);
```

### 4.2 FrameGraph::Execute() 流程

```cpp
RenderCommandList FrameGraph::Execute(
    const FrameGraphExecutionPlan& plan, 
    const RenderCommandList* sceneCommands
) {
    RenderCommandList commandList;  // 最终的命令列表
    
    FrameGraphBuilder builder(this);
    
    // 按照执行顺序遍历节点
    const auto& executionOrder = plan.GetExecutionOrder();  // [0, 1, 2]
    for (uint32_t nodeIndex : executionOrder) {
        auto* node = m_Nodes[nodeIndex].get();
        
        if (node && node->GetExecuteCallback()) {
            // 调用节点的 ExecuteCallback
            // 传入 builder（用于访问资源）和 sceneCommands（场景渲染命令）
            RenderCommandList nodeCommands = node->GetExecuteCallback()(builder, sceneCommands);
            
            // 合并节点命令到主命令列表
            for (const auto& cmd : nodeCommands.GetCommands()) {
                commandList.AddCommand(cmd);
            }
        }
    }
    
    return commandList;
}
```

### 4.3 ExecuteCallback 执行过程

**以 GeometryPass 为例**:

```cpp
// 这个 Lambda 在 Execute() 中被调用
[](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
    RenderCommandList cmdList;
    
    // 1. 可以通过 builder 访问资源
    // RHI::IImage* albedo = builder.WriteTexture("GBufferAlbedo");
    
    // 2. 合并场景命令（来自 SceneRenderer）
    if (sceneCommands && !sceneCommands->IsEmpty()) {
        for (const auto& cmd : sceneCommands->GetCommands()) {
            // cmd 是 RenderCommand 对象（数据）
            // 例如: { type: BindPipeline, params: { pipeline } }
            cmdList.AddCommand(cmd);
        }
    }
    
    // 3. 返回 RenderCommandList（数据，不是 GPU 命令）
    return cmdList;
}
```

**sceneCommands 的内容**（来自 `SceneRenderer::SubmitRenderQueue()`）:
```cpp
RenderCommandList {
    RenderCommand[] {
        { type: BindPipeline, params: { pipeline: IPipeline* } },
        { type: BindVertexBuffers, params: { buffers: [...], offsets: [...] } },
        { type: BindIndexBuffer, params: { buffer: IBuffer*, offset: 0 } },
        { type: DrawIndexed, params: { indexCount: 36, ... } },
        ...
    }
}
```

**输出**: `RenderCommandList` (m_FrameGraphRenderCommands)
```cpp
RenderCommandList {
    RenderCommand[] {
        // Geometry Pass 的命令
        { type: BindPipeline, ... },
        { type: BindVertexBuffers, ... },
        { type: DrawIndexed, ... },
        
        // Lighting Pass 的命令（如果有）
        { type: BindPipeline, ... },
        ...
        
        // PostProcess Pass 的命令（如果有）
        ...
    }
}
```

**此时的状态**:
- `RenderCommandList` 包含所有 Pass 的渲染命令
- 命令是**数据形式**（RenderCommand 对象）
- **不是 GPU 命令**，不能直接提交给 GPU

---

## 阶段五：CommandRecorder - 转换为 GPU 命令

### 5.1 调用入口

**位置**: `OnSubmit()` → `CommandRecorder::RecordCommands()`

```cpp
// RenderApp::OnSubmit()
m_CommandBuffer->Begin();

// 使用 CommandRecorder 将 RenderCommandList 转换为 GPU 命令
m_CommandRecorder.RecordCommands(m_CommandBuffer.get(), m_FrameGraphRenderCommands);
```

### 5.2 CommandRecorder::RecordCommands() 流程

```cpp
void CommandRecorder::RecordCommands(
    RHI::ICommandBuffer* commandBuffer,
    const RenderCommandList& commandList
) {
    // 遍历每个 RenderCommand
    const auto& commands = commandList.GetCommands();
    for (const auto& command : commands) {
        RecordCommand(commandBuffer, command);
    }
}
```

### 5.3 RecordCommand() - 命令类型分发

```cpp
void CommandRecorder::RecordCommand(
    RHI::ICommandBuffer* commandBuffer,
    const RenderCommand& command
) {
    switch (command.type) {
        case RenderCommandType::BindPipeline:
            // 调用实际的 CommandBuffer API
            commandBuffer->BindPipeline(command.params.bindPipeline.pipeline);
            break;
            
        case RenderCommandType::BindVertexBuffers:
            commandBuffer->BindVertexBuffers(
                command.params.bindVertexBuffers.firstBinding,
                command.params.bindVertexBuffers.buffers,
                command.params.bindVertexBuffers.offsets
            );
            break;
            
        case RenderCommandType::DrawIndexed:
            commandBuffer->DrawIndexed(
                command.params.drawIndexed.indexCount,
                command.params.drawIndexed.instanceCount,
                command.params.drawIndexed.firstIndex,
                command.params.drawIndexed.vertexOffset,
                command.params.drawIndexed.firstInstance
            );
            break;
            
        case RenderCommandType::BeginRenderPass:
            commandBuffer->BeginRenderPass(
                command.params.beginRenderPass.renderPass,
                command.params.beginRenderPass.framebuffer,
                command.params.beginRenderPass.clearColors,
                command.params.beginRenderPass.clearDepth,
                command.params.beginRenderPass.clearStencil
            );
            break;
            
        // ... 其他命令类型
    }
}
```

**此时**: 命令已记录到 `ICommandBuffer`（真正的 GPU 命令）

---

## 阶段六：SubmitCommandBuffer() - 提交到 GPU

### 6.1 提交流程

**位置**: `OnSubmit()`

```cpp
// 1. 结束记录
m_CommandBuffer->End();

// 2. 提交到 GPU 队列
std::vector<SemaphoreHandle> waitSemaphores = {m_ImageAvailableSemaphore};
std::vector<SemaphoreHandle> signalSemaphores = {m_RenderFinishedSemaphore};
m_Device->SubmitCommandBuffer(
    m_CommandBuffer.get(),
    waitSemaphores,
    signalSemaphores,
    m_InFlightFence
);

// 3. 呈现到屏幕
m_Swapchain->Present(m_CurrentImageIndex, {m_RenderFinishedSemaphore});
```

---

## 完整数据转换链

```
1. BuildFrameGraph()
   └─ FrameGraphNode (结构)
       ├─ m_Name: "GeometryPass"
       ├─ m_Type: "geometry"
       ├─ m_ReadResources: []
       ├─ m_WriteResources: ["GBufferAlbedo", ...]
       └─ m_ExecuteCallback: Lambda 函数（未执行）

2. BuildExecutionPlan()
   └─ FrameGraphExecutionPlan (执行计划)
       ├─ NodePlans: [节点计划数组]
       ├─ ResourcePlans: [资源计划数组]
       └─ ExecutionOrder: [0, 1, 2]

3. Compile()
   └─ FrameGraphResource (资源已分配)
       └─ m_RHIImage: IImage* (实际的 GPU 资源)

4. Execute()
   └─ 调用 ExecuteCallback
       └─ RenderCommandList (数据)
           └─ RenderCommand[]
               ├─ { type: BindPipeline, params: {...} }
               ├─ { type: BindVertexBuffers, params: {...} }
               └─ { type: DrawIndexed, params: {...} }

5. CommandRecorder::RecordCommands()
   └─ 遍历 RenderCommandList
       └─ 调用 ICommandBuffer API
           ├─ commandBuffer->BindPipeline(...)
           ├─ commandBuffer->BindVertexBuffers(...)
           └─ commandBuffer->DrawIndexed(...)

6. SubmitCommandBuffer()
   └─ GPU 队列
       └─ GPU 执行
           └─ 屏幕显示
```

---

## 关键数据结构

### FrameGraphNode
```cpp
class FrameGraphNode {
    std::string m_Name;                          // 节点名称
    std::string m_Type;                          // 节点类型
    std::vector<std::string> m_ReadResources;    // 读取的资源
    std::vector<std::string> m_WriteResources;   // 写入的资源
    ExecuteCallback m_ExecuteCallback;            // 执行回调（Lambda）
};
```

### RenderCommand
```cpp
class RenderCommand {
    RenderCommandType type;                       // 命令类型
    union {
        BindPipelineParams bindPipeline;
        BindVertexBuffersParams bindVertexBuffers;
        DrawIndexedParams drawIndexed;
        BeginRenderPassParams beginRenderPass;
        // ...
    } params;
};
```

### RenderCommandList
```cpp
class RenderCommandList {
    std::vector<RenderCommand> m_Commands;         // 命令数组
};
```

---

## ExecuteCallback 的详细说明

### ExecuteCallback 的类型

```cpp
using ExecuteCallback = std::function<RenderCommandList(FrameGraphBuilder&, const RenderCommandList*)>;
```

### ExecuteCallback 的作用

1. **生成 Pass 特定的命令**
   - Geometry Pass: 生成 BeginRenderPass、场景渲染命令、EndRenderPass
   - Lighting Pass: 生成光照计算命令
   - PostProcess Pass: 生成后处理命令

2. **访问 FrameGraph 资源**
   - 通过 `FrameGraphBuilder` 访问已分配的资源
   - 例如: `RHI::IImage* albedo = builder.WriteTexture("GBufferAlbedo");`

3. **合并场景命令**
   - 接收 `sceneCommands` 参数
   - 将场景渲染命令合并到 Pass 中

### ExecuteCallback 的执行时机

- **不在 BuildFrameGraph() 时执行**
- **在 Execute() 时执行**
- 此时资源已分配，可以安全访问

---

## 完整示例：Geometry Pass 的完整流程

### 1. BuildFrameGraph() 阶段

```cpp
// DeferredRenderPipeline::AddGeometryPass()
auto* node = frameGraph.AddNode("GeometryPass");
node->SetType("geometry");
node->AddWriteResource("GBufferAlbedo");

node->SetExecuteCallback([](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
    RenderCommandList cmdList;
    
    // 访问资源（此时资源已分配）
    RHI::IImage* albedo = builder.WriteTexture("GBufferAlbedo");
    
    // 生成 BeginRenderPass 命令
    RenderCommand beginCmd;
    beginCmd.type = RenderCommandType::BeginRenderPass;
    beginCmd.params.beginRenderPass.renderPass = ...;
    beginCmd.params.beginRenderPass.framebuffer = ...;
    cmdList.AddCommand(beginCmd);
    
    // 合并场景命令
    if (sceneCommands) {
        for (const auto& cmd : sceneCommands->GetCommands()) {
            cmdList.AddCommand(cmd);
        }
    }
    
    // 生成 EndRenderPass 命令
    RenderCommand endCmd;
    endCmd.type = RenderCommandType::EndRenderPass;
    cmdList.AddCommand(endCmd);
    
    return cmdList;
});
```

### 2. Execute() 阶段

```cpp
// FrameGraph::Execute()
for (uint32_t nodeIndex : executionOrder) {
    auto* node = m_Nodes[nodeIndex].get();
    
    // 调用 ExecuteCallback
    RenderCommandList nodeCommands = node->GetExecuteCallback()(builder, sceneCommands);
    
    // 合并到主命令列表
    commandList.AddCommand(nodeCommands);
}
```

### 3. CommandRecorder 阶段

```cpp
// CommandRecorder::RecordCommands()
for (const auto& command : commands) {
    switch (command.type) {
        case RenderCommandType::BeginRenderPass:
            commandBuffer->BeginRenderPass(...);
            break;
        case RenderCommandType::BindPipeline:
            commandBuffer->BindPipeline(...);
            break;
        case RenderCommandType::DrawIndexed:
            commandBuffer->DrawIndexed(...);
            break;
        case RenderCommandType::EndRenderPass:
            commandBuffer->EndRenderPass();
            break;
    }
}
```

---

## 关键设计点

### 1. 延迟执行（Lazy Execution）

- `ExecuteCallback` 在 `BuildFrameGraph()` 时**设置**，但不执行
- 在 `Execute()` 时才**调用**
- 此时资源已分配，可以安全访问

### 2. 数据与命令分离

- `BuildFrameGraph()` → 生成**结构**（Node、Resource）
- `Execute()` → 生成**数据**（RenderCommandList）
- `CommandRecorder` → 转换为**GPU命令**（ICommandBuffer）

### 3. 资源访问时机

- `BuildFrameGraph()`: 只能访问资源**描述**（width, height, format）
- `Execute()`: 可以访问**实际资源**（IImage*, IBuffer*）
- 通过 `FrameGraphBuilder` 安全访问

### 4. 场景命令合并

- 场景命令在 `OnRender()` 中生成
- 传递给 `FrameGraph::Execute()`
- 在 `ExecuteCallback` 中合并到对应的 Pass

---

## 总结

整个流程实现了**完全的解耦**:

1. **结构定义** (`BuildFrameGraph`): 定义渲染管线的结构
2. **依赖分析** (`BuildExecutionPlan`): 分析节点依赖和执行顺序
3. **资源分配** (`Compile`): 分配实际的 GPU 资源
4. **命令生成** (`Execute`): 调用 ExecuteCallback 生成命令数据
5. **命令转换** (`CommandRecorder`): 将数据转换为 GPU 命令
6. **命令提交** (`SubmitCommandBuffer`): 提交到 GPU 队列

这种设计使得系统具有高度的**灵活性**和**可扩展性**，便于动态调整渲染管线和多线程优化。
