# Render Pass 重构说明

本文档说明了对 RenderPipeline 中 Pass 管理的重构，将每个 Pass 抽象为独立的类，并使用枚举类型替代字符串。

---

## 重构目标

1. **每个 Pass 有具体的类来管理**：不再在 RenderPipeline 中直接调用 `frameGraph.AddResource()` 和 `node->AddWriteResource()`
2. **实现 IRenderPass 基类**：包含每个 Pass 共用的方法
3. **每个 Pass 的具体实现统一管理资源**：在 Pass 类中统一设置 `AddResource` 和 `AddWriteResource`
4. **使用枚举类型**：Node 的 Type 和 Resource 使用枚举类型，而不是字符串

---

## 新增文件

### 1. 枚举类型定义

**`include/FirstEngine/Renderer/RenderPassTypes.h`** 和 **`src/Renderer/RenderPassTypes.cpp`**

定义了以下枚举类型：

- **`RenderPassType`**: Pass 类型枚举
  - `Unknown`, `Geometry`, `Lighting`, `Forward`, `PostProcess`, `Shadow`, `Skybox`, `UI`, `Custom`

- **`FrameGraphResourceName`**: 资源名称枚举
  - `GBufferAlbedo`, `GBufferNormal`, `GBufferDepth`, `GBufferRoughness`, `GBufferMetallic`
  - `FinalOutput`, `HDRBuffer`, `ShadowMap`, `PostProcessBuffer`, `Custom`

提供了字符串转换函数：
- `RenderPassTypeToString()` / `StringToRenderPassType()`
- `FrameGraphResourceNameToString()` / `StringToFrameGraphResourceName()`

### 2. IRenderPass 基类

**`include/FirstEngine/Renderer/IRenderPass.h`** 和 **`src/Renderer/IRenderPass.cpp`**

抽象基类，定义了每个 Pass 必须实现的接口：

```cpp
class IRenderPass {
public:
    // 构建 Pass：一次性完成资源添加和节点配置
    // frameGraph: 要添加资源的 FrameGraph
    // node: 为此 Pass 创建的 FrameGraphNode（可能为 nullptr）
    // resolution: 当前渲染分辨率（用于资源大小设置）
    virtual void OnBuild(FrameGraph& frameGraph, FrameGraphNode* node, const ResolutionConfig& resolution) = 0;
    
    // 获取执行回调（生成渲染命令）
    virtual FrameGraphNode::ExecuteCallback GetExecuteCallback() = 0;
};
```

**注意**：`OnBuild` 方法将原来的 `AddResources` 和 `ConfigureNode` 合并为一个方法，减少了方法调用和参数传递。

### 3. 具体 Pass 实现

**GeometryPass** (`include/FirstEngine/Renderer/GeometryPass.h` / `src/Renderer/GeometryPass.cpp`)
- 管理 G-Buffer 资源（Albedo, Normal, Depth）
- 配置 Geometry Pass 节点

**LightingPass** (`include/FirstEngine/Renderer/LightingPass.h` / `src/Renderer/LightingPass.cpp`)
- 管理最终输出资源
- 配置 Lighting Pass 节点（读取 G-Buffer，写入 FinalOutput）

**PostProcessPass** (`include/FirstEngine/Renderer/PostProcessPass.h` / `src/Renderer/PostProcessPass.cpp`)
- 管理最终输出资源
- 配置 PostProcess Pass 节点（读写 FinalOutput）

---

## 修改的文件

### 1. FrameGraph.h / FrameGraph.cpp

**修改内容**：
- `FrameGraphNode::SetType()` 和 `GetType()` 现在使用 `RenderPassType` 枚举
- 保留了字符串版本的 `SetType(const std::string&)` 和 `GetTypeString()` 用于向后兼容
- 内部存储从 `std::string m_Type` 改为 `RenderPassType m_Type`

**代码示例**：
```cpp
// 使用枚举
node->SetType(RenderPassType::Geometry);

// 或使用字符串（向后兼容）
node->SetType("geometry");
```

### 2. FrameGraphExecutionPlan.h

**修改内容**：
- `NodePlan::type` 从 `std::string` 改为 `RenderPassType`
- 添加了 `NodePlan::typeString` 用于序列化/调试

### 3. DeferredRenderPipeline.h / DeferredRenderPipeline.cpp

**重构前**：
```cpp
void DeferredRenderPipeline::AddGeometryPass(FrameGraph& frameGraph) {
    auto* node = frameGraph.AddNode("GeometryPass");
    node->SetType("geometry");
    node->AddWriteResource("GBufferAlbedo");
    // ...
}
```

**重构后**：
```cpp
bool DeferredRenderPipeline::BuildFrameGraph(...) {
    if (pipelineConfig.deferredSettings.geometryPassEnabled) {
        auto* geometryNode = frameGraph.AddNode(m_GeometryPass->GetName());
        if (geometryNode) {
            // 一次性完成资源添加和节点配置
            m_GeometryPass->OnBuild(frameGraph, geometryNode, resolution);
            geometryNode->SetExecuteCallback(m_GeometryPass->GetExecuteCallback());
        }
    }
    // ...
}
```

**主要变化**：
- 移除了 `AddGBufferResources()`, `AddOutputResources()`, `AddGeometryPass()`, `AddLightingPass()`, `AddPostProcessPass()` 方法
- 使用 Pass 类的实例来管理资源和节点配置
- 每个 Pass 负责自己的资源添加和节点配置
- **优化**：将 `AddResources` 和 `ConfigureNode` 合并为 `OnBuild` 方法，减少方法调用和参数传递

### 4. CMakeLists.txt

添加了新的源文件和头文件：
- `RenderPassTypes.cpp`
- `IRenderPass.cpp`
- `GeometryPass.cpp`
- `LightingPass.cpp`
- `PostProcessPass.cpp`

---

## 使用示例

### 创建自定义 Pass

```cpp
class MyCustomPass : public IRenderPass {
public:
    MyCustomPass() : IRenderPass("MyCustomPass", RenderPassType::Custom) {}
    
    void OnBuild(FrameGraph& frameGraph, FrameGraphNode* node, const ResolutionConfig& resolution) override {
        // 添加资源
        ResourceDescription desc;
        desc.type = ResourceType::Attachment;
        desc.name = "MyCustomResource";
        desc.width = resolution.width;
        desc.height = resolution.height;
        frameGraph.AddResource("MyCustomResource", desc);
        
        // 配置节点
        if (node) {
            node->SetType(GetType()); // 使用枚举
            node->AddWriteResource("MyCustomResource");
        }
    }
    
    FrameGraphNode::ExecuteCallback GetExecuteCallback() override {
        return [](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
            RenderCommandList cmdList;
            // 生成命令...
            return cmdList;
        };
    }
};
```

### 在 RenderPipeline 中使用

```cpp
class MyRenderPipeline : public IRenderPipeline {
private:
    std::unique_ptr<MyCustomPass> m_CustomPass;
    
public:
    MyRenderPipeline() : m_CustomPass(std::make_unique<MyCustomPass>()) {}
    
    bool BuildFrameGraph(FrameGraph& frameGraph, const RenderConfig& config) override {
        // 创建节点
        auto* node = frameGraph.AddNode(m_CustomPass->GetName());
        if (node) {
            // Pass 一次性完成资源添加和节点配置
            m_CustomPass->OnBuild(frameGraph, node, config.GetResolution());
            node->SetExecuteCallback(m_CustomPass->GetExecuteCallback());
        }
        
        return true;
    }
};
```

---

## 优势

1. **职责分离**：每个 Pass 类负责管理自己的资源和配置，代码更清晰
2. **可扩展性**：添加新 Pass 只需创建新的 Pass 类，无需修改 RenderPipeline
3. **类型安全**：使用枚举类型替代字符串，减少拼写错误
4. **可维护性**：Pass 的逻辑集中在一个类中，便于维护和测试
5. **代码复用**：IRenderPass 基类提供统一的接口，便于代码复用

---

## 向后兼容性

- `FrameGraphNode::SetType()` 仍然支持字符串参数（自动转换为枚举）
- `FrameGraphNode::GetTypeString()` 提供字符串表示（用于序列化/调试）
- 现有的代码可以逐步迁移到新的 Pass 类系统

---

## 后续工作

1. 实现更多 Pass 类型（ShadowPass, SkyboxPass, UIPass 等）
2. 添加 Pass 配置验证（`IRenderPass::Validate()`）
3. 支持 Pass 之间的依赖关系配置
4. 添加 Pass 的序列化/反序列化支持
