#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        // FrameGraphNode 实现
        FrameGraphNode::FrameGraphNode(const std::string& name, uint32_t index)
            : m_Name(name), m_Index(index), m_Type(RenderPassType::Unknown), m_FrameGraph(nullptr) {
        }

        FrameGraphNode::~FrameGraphNode() = default;

        void FrameGraphNode::AddReadResource(const std::string& resourceName, const ResourceDescription* resourceDesc) {
            m_ReadResources.push_back(resourceName);

            // If FrameGraph is set and resource description is provided, automatically add and allocate resource
            // This allows Passes to declare resources they need without manually calling AddResource/AllocateResource
            if (m_FrameGraph && resourceDesc) {
                // Check if resource already exists
                auto* existingResource = m_FrameGraph->GetResource(resourceName);
                if (!existingResource) {
                    // Add resource
                    m_FrameGraph->AddResource(resourceName, *resourceDesc);
                    // Allocate resource immediately (per-pass resource allocation)
                    m_FrameGraph->AllocateResource(resourceName);
                }
            }
            // If resourceDesc is nullptr, resource should already exist (added by another Pass)
        }

        void FrameGraphNode::AddWriteResource(const std::string& resourceName, const ResourceDescription* resourceDesc) {
            m_WriteResources.push_back(resourceName);

            // If FrameGraph is set and resource description is provided, automatically add and allocate resource
            // This allows Passes to declare resources they need without manually calling AddResource/AllocateResource
            if (m_FrameGraph && resourceDesc) {
                // Check if resource already exists
                auto* existingResource = m_FrameGraph->GetResource(resourceName);
                if (!existingResource) {
                    // Add resource
                    m_FrameGraph->AddResource(resourceName, *resourceDesc);
                    // Allocate resource immediately (per-pass resource allocation)
                    m_FrameGraph->AllocateResource(resourceName);
                }
            }
            // If resourceDesc is nullptr, resource should already exist (added by another Pass)
        }

        void FrameGraphNode::AddDependency(uint32_t nodeIndex) {
            m_Dependencies.push_back(nodeIndex);
        }

        // FrameGraphResource 实现
        FrameGraphResource::FrameGraphResource(const std::string& name, const ResourceDescription& desc)
            : m_Name(name), m_Description(desc) {
        }

        FrameGraphResource::~FrameGraphResource() = default;

        // FrameGraphBuilder 实现
        FrameGraphBuilder::FrameGraphBuilder(FrameGraph* graph)
            : m_Graph(graph) {
        }

        RHI::IImage* FrameGraphBuilder::ReadTexture(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Texture) {
                return resource->GetRHIImage();
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::ReadBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Buffer) {
                return resource->GetRHIBuffer();
            }
            return nullptr;
        }

        RHI::IImage* FrameGraphBuilder::WriteTexture(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Texture) {
                return resource->GetRHIImage();
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::WriteBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Buffer) {
                return resource->GetRHIBuffer();
            }
            return nullptr;
        }

        std::string FrameGraphBuilder::CreateTexture(const std::string& name, const ResourceDescription& desc) {
            m_Graph->AddResource(name, desc);
            return name;
        }

        std::string FrameGraphBuilder::CreateBuffer(const std::string& name, const ResourceDescription& desc) {
            m_Graph->AddResource(name, desc);
            return name;
        }

        FrameGraph::FrameGraph(RHI::IDevice* device)
            : m_Device(device) {
        }

        FrameGraph::~FrameGraph() = default;

        FrameGraphNode* FrameGraph::AddNode(const std::string& name) {
            if (m_NodeNameToIndex.find(name) != m_NodeNameToIndex.end()) {
                return nullptr; 
            }

            uint32_t index = static_cast<uint32_t>(m_Nodes.size());
            auto node = std::make_unique<FrameGraphNode>(name, index);
            FrameGraphNode* nodePtr = node.get();
            
            // Set FrameGraph reference and index for automatic resource management
            nodePtr->SetFrameGraph(this);
            nodePtr->SetIndex(index);
            
            m_Nodes.push_back(std::move(node));
            m_NodeNameToIndex[name] = index;
            return nodePtr;
        }

        // Add an existing node (for IRenderPass that inherits from FrameGraphNode)
        // Note: The node's lifetime is managed by the caller (e.g., IRenderPass in DeferredRenderPipeline)
        // FrameGraph only stores a reference, not ownership
        FrameGraphNode* FrameGraph::AddNode(FrameGraphNode* node) {
            if (!node) {
                return nullptr;
            }

            const std::string& name = node->GetName();
            if (m_NodeNameToIndex.find(name) != m_NodeNameToIndex.end()) {
                return nullptr; 
            }

            uint32_t index = static_cast<uint32_t>(m_Nodes.size());
            
            // Set FrameGraph reference and index for automatic resource management
            node->SetFrameGraph(this);
            node->SetIndex(index);
            
            // If node is an IRenderPass, automatically set execute callback from OnDraw
            // This eliminates the need for Pass to manually call SetExecuteCallback
            IRenderPass* pass = dynamic_cast<IRenderPass*>(node);
            if (pass) {
                // Create a lambda that calls OnDraw
                ExecuteCallback callback = [pass](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                    return pass->OnDraw(builder, sceneCommands);
                };
                node->SetExecuteCallback(callback);
            }
            
            // Store as raw pointer (lifetime managed by caller)
            // We need to use a custom deleter that does nothing, since we don't own the node
            m_Nodes.push_back(std::unique_ptr<FrameGraphNode>(node, [](FrameGraphNode*) {}));
            m_NodeNameToIndex[name] = index;
            return node;
        }

        FrameGraphResource* FrameGraph::AddResource(const std::string& name, const ResourceDescription& desc) {
            if (m_Resources.find(name) != m_Resources.end()) {
                return nullptr; 
            }

            auto resource = std::make_unique<FrameGraphResource>(name, desc);
            FrameGraphResource* resourcePtr = resource.get();
            m_Resources[name] = std::move(resource);
            return resourcePtr;
        }

        bool FrameGraph::AllocateResource(const std::string& resourceName) {
            auto it = m_Resources.find(resourceName);
            if (it == m_Resources.end()) {
                return false;
            }

            auto* resource = it->second.get();
            const auto& desc = resource->GetDescription();

            // Check if already allocated
            ResourceType type = desc.GetType();
            if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                if (resource->GetRHIImage() != nullptr) {
                    return true; // Already allocated
                }
            } else if (type == ResourceType::Buffer) {
                if (resource->GetRHIBuffer() != nullptr) {
                    return true; // Already allocated
                }
            }

            // Allocate resource
            if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                // Create image
                RHI::ImageDescription imageDesc;
                imageDesc.width = desc.GetWidth();
                imageDesc.height = desc.GetHeight();
                imageDesc.format = desc.GetFormat();
                imageDesc.usage = (type == ResourceType::Attachment) 
                    ? RHI::ImageUsageFlags::ColorAttachment 
                    : RHI::ImageUsageFlags::Sampled;
                imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                auto image = m_Device->CreateImage(imageDesc);
                if (!image) {
                    return false;
                }

                resource->SetRHIResource(image.release());
            } else if (type == ResourceType::Buffer) {
                // Create buffer
                auto buffer = m_Device->CreateBuffer(
                    desc.GetSize(),
                    desc.GetBufferUsage(),
                    RHI::MemoryPropertyFlags::DeviceLocal
                );
                if (!buffer) {
                    return false;
                }

                resource->SetRHIResource(buffer.release());
            }

            return true;
        }

        bool FrameGraph::BuildExecutionPlan(FrameGraphExecutionPlan& plan) {
            plan.Clear();

            // Analyze dependencies first
            AnalyzeDependencies();

            // Build node plans
            for (const auto& node : m_Nodes) {
                FrameGraphExecutionPlan::NodePlan nodePlan;
                nodePlan.name = node->GetName();
                nodePlan.index = node->GetIndex();
                nodePlan.type = node->GetType(); // RenderPassType enum
                nodePlan.typeString = node->GetTypeString(); // String representation
                nodePlan.readResources = node->GetReadResources();
                nodePlan.writeResources = node->GetWriteResources();
                nodePlan.dependencies = node->GetDependencies();
                plan.AddNodePlan(std::move(nodePlan));
            }

            // Build resource plans
            for (const auto& [resourceName, resource] : m_Resources) {
                FrameGraphExecutionPlan::ResourcePlan resourcePlan;
                resourcePlan.name = resource->GetName();
                const auto& desc = resource->GetDescription();
                resourcePlan.type = desc.GetType();
                
                // Clone the ResourceDescription based on type
                ResourceType type = desc.GetType();
                if (type == ResourceType::Attachment) {
                    resourcePlan.description = std::make_unique<AttachmentResource>(
                        desc.GetName(),
                        desc.GetWidth(),
                        desc.GetHeight(),
                        desc.GetFormat(),
                        desc.HasDepth()
                    );
                } else if (type == ResourceType::Buffer) {
                    resourcePlan.description = std::make_unique<BufferResource>(
                        desc.GetName(),
                        desc.GetSize(),
                        desc.GetBufferUsage()
                    );
                } else {
                    // Fallback: create a basic ResourceDescription
                    resourcePlan.description = std::make_unique<ResourceDescription>(type, desc.GetName());
                }
                
                plan.AddResourcePlan(std::move(resourcePlan));
            }

            // Calculate execution order (topological sort)
            std::vector<uint32_t> executionOrder = TopologicalSort();
            plan.SetExecutionOrder(executionOrder);

            return plan.IsValid();
        }

        bool FrameGraph::Compile(const FrameGraphExecutionPlan& plan) {
            if (!plan.IsValid()) {
                return false;
            }

            // Allocate resources based on the execution plan
            // Resources are allocated when needed, not during plan building
            return AllocateResources();
        }

        RenderCommandList FrameGraph::Execute(const FrameGraphExecutionPlan& plan, Resources::Scene* scene) {
            RenderCommandList commandList;

            if (!plan.IsValid()) {
                return commandList;
            }

            FrameGraphBuilder builder(this);

            // Execute nodes in the order specified by the plan
            // For each node that is an IRenderPass:
            //   1. If it has a SceneRenderer, call Render() to generate SceneRenderCommands
            //   2. Get SceneRenderCommands from SceneRenderer
            //   3. Pass SceneRenderCommands to OnDraw() callback
            const auto& executionOrder = plan.GetExecutionOrder();
            for (uint32_t nodeIndex : executionOrder) {
                auto* node = m_Nodes[nodeIndex].get();
                if (!node || !node->GetExecuteCallback()) {
                    continue;
                }

                // Check if node is an IRenderPass with SceneRenderer
                IRenderPass* pass = dynamic_cast<IRenderPass*>(node);
                const RenderCommandList* sceneCommands = nullptr;

                if (pass && pass->HasSceneRenderer() && scene) {
                    // Get SceneRenderer from pass
                    SceneRenderer* sceneRenderer = pass->GetSceneRenderer();
                    
                    // Determine camera config (use custom camera if set, otherwise use global from RenderConfig)
                    CameraConfig cameraConfig;
                    if (pass->UsesCustomCamera()) {
                        cameraConfig = pass->GetCameraConfig();
                    } else {
                        // TODO: Get global camera from RenderConfig (needs to be passed to Execute)
                        // For now, use default camera
                    }

                    // Get resolution config (from RenderConfig, needs to be passed)
                    ResolutionConfig resolutionConfig;
                    resolutionConfig.width = 1280;
                    resolutionConfig.height = 720;

                    // Get render flags (from RenderConfig, needs to be passed)
                    RenderFlags renderFlags;
                    renderFlags.frustumCulling = true;
                    renderFlags.occlusionCulling = false;

                    // Render scene using SceneRenderer
                    sceneRenderer->Render(scene, cameraConfig, resolutionConfig, renderFlags);

                    // Get generated commands from SceneRenderer
                    if (sceneRenderer->HasRenderCommands()) {
                        sceneCommands = &sceneRenderer->GetRenderCommands();
                    }
                }

                // Get command list from node callback, passing scene commands
                // Geometry and forward passes can merge scene commands into their pass
                RenderCommandList nodeCommands = node->GetExecuteCallback()(builder, sceneCommands);

                // Merge node commands into main command list
                const auto& nodeCmdList = nodeCommands.GetCommands();
                for (const auto& cmd : nodeCmdList) {
                    commandList.AddCommand(cmd);
                }
            }

            return commandList;
        }

        FrameGraphResource* FrameGraph::GetResource(const std::string& name) {
            auto it = m_Resources.find(name);
            return (it != m_Resources.end()) ? it->second.get() : nullptr;
        }

        const FrameGraphResource* FrameGraph::GetResource(const std::string& name) const {
            auto it = m_Resources.find(name);
            return (it != m_Resources.end()) ? it->second.get() : nullptr;
        }

        FrameGraphNode* FrameGraph::GetNode(uint32_t index) {
            if (index < m_Nodes.size()) {
                return m_Nodes[index].get();
            }
            return nullptr;
        }

        const FrameGraphNode* FrameGraph::GetNode(uint32_t index) const {
            if (index < m_Nodes.size()) {
                return m_Nodes[index].get();
            }
            return nullptr;
        }

        void FrameGraph::Clear() {
            // Note: Resources should be released explicitly before Clear() if needed
            // Clear() only clears the graph structure, not the resources
            // This allows ReleaseResources() to be called separately for better control
            
            m_Nodes.clear();
            m_Resources.clear();
            m_ResourceNameToIndex.clear();
            m_NodeNameToIndex.clear();
        }

        void FrameGraph::ReleaseResources() {
            // Release all allocated RHI resources
            // Resources are managed by unique_ptr, so we need to delete them manually
            for (auto& [resourceName, resource] : m_Resources) {
                if (resource) {
                    const auto& desc = resource->GetDescription();
                    ResourceType type = desc.GetType();
                    
                    if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                        // Release image
                        RHI::IImage* image = resource->GetRHIImage();
                        if (image) {
                            // Delete the image (destructor will handle cleanup)
                            delete image;
                        }
                    } else if (type == ResourceType::Buffer) {
                        // Release buffer
                        RHI::IBuffer* buffer = resource->GetRHIBuffer();
                        if (buffer) {
                            // Delete the buffer (destructor will handle cleanup)
                            delete buffer;
                        }
                    }
                    
                    // Clear RHI resource pointers
                    resource->SetRHIResource(static_cast<RHI::IImage*>(nullptr));
                    resource->SetRHIResource(static_cast<RHI::IBuffer*>(nullptr));
                }
            }
        }

        void FrameGraph::AnalyzeDependencies() {
            
            for (auto& [resourceName, resource] : m_Resources) {
                uint32_t firstUse = UINT32_MAX;
                uint32_t lastUse = 0;

                for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                    auto* node = m_Nodes[i].get();
                    bool used = false;

                    // 检查读取
                    for (const auto& readRes : node->GetReadResources()) {
                        if (readRes == resourceName) {
                            used = true;
                            break;
                        }
                    }

                    // 检查写入
                    if (!used) {
                        for (const auto& writeRes : node->GetWriteResources()) {
                            if (writeRes == resourceName) {
                                used = true;
                                break;
                            }
                        }
                    }

                    if (used) {
                        firstUse = std::min(firstUse, i);
                        lastUse = std::max(lastUse, i);
                    }
                }

                // 更新资源描述
                auto& desc = const_cast<ResourceDescription&>(resource->GetDescription());
                desc.SetFirstPass(firstUse);
                desc.SetLastPass(lastUse);
            }

            // 建立节点之间的依赖关系
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                auto* node = m_Nodes[i].get();

                // 对于每个写入的资源，找到最后写入它的节点
                for (const auto& writeRes : node->GetWriteResources()) {
                    auto* resource = GetResource(writeRes);
                    if (resource) {
                        // 找到所有读取此资源的后续节点
                        for (uint32_t j = i + 1; j < m_Nodes.size(); ++j) {
                            auto* otherNode = m_Nodes[j].get();
                            for (const auto& readRes : otherNode->GetReadResources()) {
                                if (readRes == writeRes) {
                                    otherNode->AddDependency(i);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        bool FrameGraph::AllocateResources() {
            // 为每个资源分配实际的 RHI 资源
            // Note: Resources may have already been allocated by individual Passes
            // This method will skip already-allocated resources
            for (auto& [resourceName, resource] : m_Resources) {
                const auto& desc = resource->GetDescription();
                ResourceType type = desc.GetType();

                // Check if already allocated
                bool alreadyAllocated = false;
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    if (resource->GetRHIImage() != nullptr) {
                        alreadyAllocated = true;
                    }
                } else if (type == ResourceType::Buffer) {
                    if (resource->GetRHIBuffer() != nullptr) {
                        alreadyAllocated = true;
                    }
                }

                if (alreadyAllocated) {
                    continue; // Skip already allocated resources
                }

                // Allocate resource
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    // 创建图像
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = desc.GetWidth();
                    imageDesc.height = desc.GetHeight();
                    imageDesc.format = desc.GetFormat();
                    imageDesc.usage = (type == ResourceType::Attachment) 
                        ? RHI::ImageUsageFlags::ColorAttachment 
                        : RHI::ImageUsageFlags::Sampled;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto image = m_Device->CreateImage(imageDesc);
                    if (!image) {
                        return false;
                    }

                    // 存储图像指针，FrameGraph 负责管理生命周期
                    resource->SetRHIResource(image.release());
                } else if (type == ResourceType::Buffer) {
                    // 创建缓冲区
                    auto buffer = m_Device->CreateBuffer(
                        desc.GetSize(),
                        desc.GetBufferUsage(),
                        RHI::MemoryPropertyFlags::DeviceLocal
                    );
                    if (!buffer) {
                        return false;
                    }

                    // 存储缓冲区指针，FrameGraph 负责管理生命周期
                    resource->SetRHIResource(buffer.release());
                }
            }

            return true;
        }

        std::vector<uint32_t> FrameGraph::TopologicalSort() {
            std::vector<uint32_t> result;
            std::vector<int> inDegree(m_Nodes.size(), 0);

            // 计算入度
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                auto* node = m_Nodes[i].get();
                for (uint32_t dep : node->GetDependencies()) {
                    inDegree[i]++;
                }
            }

            // Kahn 算法
            std::queue<uint32_t> queue;
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                if (inDegree[i] == 0) {
                    queue.push(i);
                }
            }

            while (!queue.empty()) {
                uint32_t u = queue.front();
                queue.pop();
                result.push_back(u);

                // 减少依赖此节点的节点的入度
                for (uint32_t v = 0; v < m_Nodes.size(); ++v) {
                    if (v == u) continue;
                    auto* node = m_Nodes[v].get();
                    for (uint32_t dep : node->GetDependencies()) {
                        if (dep == u) {
                            inDegree[v]--;
                            if (inDegree[v] == 0) {
                                queue.push(v);
                            }
                        }
                    }
                }
            }

            // 检查是否有环
            if (result.size() != m_Nodes.size()) {
                throw std::runtime_error("FrameGraph contains cycles!");
            }

            return result;
        }

    } // namespace Renderer
} // namespace FirstEngine
