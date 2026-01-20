# IRenderPass 继承自 FrameGraphNode 重构说明

本文档说明了将 `IRenderPass` 重构为继承自 `FrameGraphNode` 的设计，进一步简化了参数传递和资源管理。

---

## 重构目标

1. **IRenderPass 继承自 FrameGraphNode**：每个 Pass 本身就是一个 Node
2. **OnBuild 中将自身加入 FrameGraph**：Pass 在 OnBuild 中调用 `frameGraph.AddNode(this)` 将自己添加
3. **自动资源管理**：`AddReadResource`/`AddWriteResource` 内部自动调用 `AddResource` 和 `AllocateResource`
4. **减少参数传递**：通过 pipeline 指针访问 RenderConfig，无需传递简单变量

---

## 主要变更

### 1. FrameGraphNode 增强

**新增功能**：
- `SetFrameGraph(FrameGraph*)`：设置 FrameGraph 引用，用于自动资源管理
- `AddReadResource(resourceName, resourceDesc)`：如果提供 `resourceDesc`，自动添加和分配资源
- `AddWriteResource(resourceName, resourceDesc)`：如果提供 `resourceDesc`，自动添加和分配资源
- `SetIndex(uint32_t)`：允许稍后设置节点索引
- 构造函数支持默认 index：`FrameGraphNode(name, index = 0xFFFFFFFF)`

**代码示例**：
```cpp
// FrameGraphNode 现在可以自动管理资源
void FrameGraphNode::AddWriteResource(const std::string& resourceName, const ResourceDescription* resourceDesc) {
    m_WriteResources.push_back(resourceName);
    
    // 如果提供了 resourceDesc，自动添加和分配资源
    if (m_FrameGraph && resourceDesc) {
        auto* existingResource = m_FrameGraph->GetResource(resourceName);
        if (!existingResource) {
            m_FrameGraph->AddResource(resourceName, *resourceDesc);
            m_FrameGraph->AllocateResource(resourceName);
        }
    }
}
```

### 2. FrameGraph::AddNode 重载

**新增方法**：
```cpp
// 添加已存在的节点（用于 IRenderPass）
FrameGraphNode* AddNode(FrameGraphNode* node);
```

**实现细节**：
- 使用自定义 deleter（空操作），因为 FrameGraph 不拥有节点的生命周期
- 节点的生命周期由调用者管理（如 DeferredRenderPipeline 中的 unique_ptr<IRenderPass>）
- 自动设置 FrameGraph 引用和索引

### 3. IRenderPass 继承自 FrameGraphNode

**之前**：
```cpp
class IRenderPass {
    std::string m_Name;
    RenderPassType m_Type;
    // ...
};
```

**现在**：
```cpp
class IRenderPass : public FrameGraphNode {
    // 继承自 FrameGraphNode，所以 Pass 本身就是 Node
    // m_Name, m_Type 等都在 FrameGraphNode 中
};
```

**优势**：
- Pass 可以直接调用 `AddReadResource`/`AddWriteResource`
- Pass 可以直接配置自己（`SetType`, `SetExecuteCallback`）
- 无需创建单独的 Node 对象

### 4. OnBuild 简化

**之前**：
```cpp
void GeometryPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
    const auto& resolution = deferredPipeline->GetRenderConfig().GetResolution();
    
    // 手动添加资源
    frameGraph.AddResource("GBufferAlbedo", albedoDesc);
    frameGraph.AllocateResource("GBufferAlbedo");
    
    // 创建并配置节点
    auto* node = frameGraph.AddNode(GetName());
    node->SetType(GetType());
    node->AddWriteResource("GBufferAlbedo");
    node->SetExecuteCallback(GetExecuteCallback());
}
```

**现在**：
```cpp
void GeometryPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
    const auto& resolution = deferredPipeline->GetRenderConfig().GetResolution();
    
    // 将自己添加到 FrameGraph
    frameGraph.AddNode(this);
    
    // 准备资源描述
    ResourceDescription albedoDesc = { ... };
    
    // 直接调用 AddWriteResource（自动添加和分配资源）
    AddWriteResource("GBufferAlbedo", &albedoDesc);
    
    // 直接配置自己
    SetExecuteCallback(GetExecuteCallback());
}
```

---

## 完整示例：GeometryPass

### GeometryPass.h
```cpp
class GeometryPass : public IRenderPass {
public:
    GeometryPass();
    void OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) override;
    ExecuteCallback GetExecuteCallback() override;
};
```

### GeometryPass.cpp
```cpp
GeometryPass::GeometryPass()
    : IRenderPass("GeometryPass", RenderPassType::Geometry) {
    // IRenderPass 构造函数会调用 FrameGraphNode(name)
    // 并设置类型
}

void GeometryPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
    // 获取 RenderConfig
    auto* deferredPipeline = dynamic_cast<DeferredRenderPipeline*>(pipeline);
    const auto& resolution = deferredPipeline->GetRenderConfig().GetResolution();
    
    // 将自己添加到 FrameGraph（this 是 IRenderPass，继承自 FrameGraphNode）
    frameGraph.AddNode(this);
    
    // 准备资源描述
    ResourceDescription albedoDesc;
    albedoDesc.type = ResourceType::Attachment;
    albedoDesc.name = "GBufferAlbedo";
    albedoDesc.width = resolution.width;
    albedoDesc.height = resolution.height;
    // ...
    
    // 直接调用 AddWriteResource（自动添加和分配资源）
    AddWriteResource("GBufferAlbedo", &albedoDesc);
    AddWriteResource("GBufferNormal", &normalDesc);
    AddWriteResource("GBufferDepth", &depthDesc);
    
    // 直接配置自己
    SetExecuteCallback(GetExecuteCallback());
}
```

---

## 资源管理流程

### 自动资源管理

当 Pass 调用 `AddWriteResource(resourceName, &resourceDesc)` 时：

1. **添加到 ReadResources/WriteResources 列表**
2. **检查 FrameGraph 引用是否设置**（在 `AddNode` 时已设置）
3. **检查资源是否已存在**
4. **如果不存在且提供了 resourceDesc**：
   - 调用 `frameGraph.AddResource(resourceName, *resourceDesc)`
   - 调用 `frameGraph.AllocateResource(resourceName)`

### 资源共享

- 如果资源已存在（由其他 Pass 添加），不会重复添加
- 多个 Pass 可以安全地声明对同一资源的访问

**示例**：
```cpp
// GeometryPass 添加并分配 GBufferAlbedo
AddWriteResource("GBufferAlbedo", &albedoDesc);

// LightingPass 读取 GBufferAlbedo（不提供 resourceDesc，因为已存在）
AddReadResource("GBufferAlbedo"); // resourceDesc = nullptr
```

---

## 优势

1. **代码更简洁**：
   - 无需手动调用 `frameGraph.AddResource()` 和 `frameGraph.AllocateResource()`
   - 无需创建和配置单独的 Node 对象
   - Pass 直接管理自己

2. **参数传递更少**：
   - 不再传递 `resolution`，通过 pipeline 访问 RenderConfig
   - `AddWriteResource` 自动处理资源添加和分配

3. **职责更清晰**：
   - Pass 完全负责自己的资源管理
   - FrameGraphNode 提供自动资源管理功能

4. **类型安全**：
   - Pass 继承自 FrameGraphNode，类型关系更明确
   - 编译时检查更严格

---

## 关键设计点

### 1. 节点生命周期管理

- **FrameGraph 不拥有 IRenderPass 的生命周期**
- IRenderPass 由 DeferredRenderPipeline 的 `unique_ptr` 管理
- FrameGraph::AddNode(FrameGraphNode*) 使用自定义 deleter（空操作）

### 2. 资源描述参数

- **提供 resourceDesc**：资源不存在时自动添加和分配
- **不提供 resourceDesc（nullptr）**：资源应该已存在（由其他 Pass 添加）

### 3. FrameGraph 引用

- 在 `AddNode` 时自动设置 `m_FrameGraph` 引用
- 使 `AddReadResource`/`AddWriteResource` 能够自动管理资源

---

## 向后兼容性

- `FrameGraph::AddNode(const std::string&)` 仍然可用（创建新节点）
- `FrameGraphNode::AddReadResource(resourceName)` 仍然可用（不提供 resourceDesc）
- `FrameGraphNode::AddWriteResource(resourceName)` 仍然可用（不提供 resourceDesc）

---

## 总结

通过让 `IRenderPass` 继承自 `FrameGraphNode`，我们实现了：

1. ✅ Pass 本身就是 Node，无需创建单独的 Node 对象
2. ✅ 在 OnBuild 中，Pass 将自己添加到 FrameGraph
3. ✅ `AddReadResource`/`AddWriteResource` 自动管理资源添加和分配
4. ✅ 减少了参数传递（通过 pipeline 访问 RenderConfig）
5. ✅ 代码更简洁，职责更清晰

这种设计使得每个 Pass 完全自包含，能够独立管理自己的资源和配置。
