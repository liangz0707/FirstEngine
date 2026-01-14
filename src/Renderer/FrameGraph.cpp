#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <algorithm>
#include <queue>
#include <stdexcept>

namespace FirstEngine {
    namespace Renderer {

        // FrameGraphNode 实现
        FrameGraphNode::FrameGraphNode(const std::string& name, uint32_t index)
            : m_Name(name), m_Index(index) {
        }

        FrameGraphNode::~FrameGraphNode() = default;

        void FrameGraphNode::AddReadResource(const std::string& resourceName) {
            m_ReadResources.push_back(resourceName);
        }

        void FrameGraphNode::AddWriteResource(const std::string& resourceName) {
            m_WriteResources.push_back(resourceName);
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
            if (resource && resource->GetDescription().type == ResourceType::Texture) {
                return resource->GetRHIImage();
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::ReadBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().type == ResourceType::Buffer) {
                return resource->GetRHIBuffer();
            }
            return nullptr;
        }

        RHI::IImage* FrameGraphBuilder::WriteTexture(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().type == ResourceType::Texture) {
                return resource->GetRHIImage();
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::WriteBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().type == ResourceType::Buffer) {
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
            m_Nodes.push_back(std::move(node));
            m_NodeNameToIndex[name] = index;
            return nodePtr;
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

        bool FrameGraph::Compile() {
            
            AnalyzeDependencies();

            
            if (!AllocateResources()) {
                return false;
            }

            return true;
        }

        void FrameGraph::Execute(RHI::ICommandBuffer* commandBuffer) {
           
            std::vector<uint32_t> executionOrder = TopologicalSort();

            FrameGraphBuilder builder(this);

            
            for (uint32_t nodeIndex : executionOrder) {
                auto* node = m_Nodes[nodeIndex].get();
                if (node && node->GetExecuteCallback()) {
                    node->GetExecuteCallback()(commandBuffer, builder);
                }
            }
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
            m_Nodes.clear();
            m_Resources.clear();
            m_ResourceNameToIndex.clear();
            m_NodeNameToIndex.clear();
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
                desc.firstPass = firstUse;
                desc.lastPass = lastUse;
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
            for (auto& [resourceName, resource] : m_Resources) {
                const auto& desc = resource->GetDescription();

                if (desc.type == ResourceType::Texture || desc.type == ResourceType::Attachment) {
                    // 创建图像
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = desc.width;
                    imageDesc.height = desc.height;
                    imageDesc.format = desc.format;
                    imageDesc.usage = (desc.type == ResourceType::Attachment) 
                        ? RHI::ImageUsageFlags::ColorAttachment 
                        : RHI::ImageUsageFlags::Sampled;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto image = m_Device->CreateImage(imageDesc);
                    if (!image) {
                        return false;
                    }

                    // 存储图像指针，FrameGraph 负责管理生命周期
                    resource->SetRHIResource(image.release());
                } else if (desc.type == ResourceType::Buffer) {
                    // 创建缓冲区
                    auto buffer = m_Device->CreateBuffer(
                        desc.size,
                        desc.bufferUsage,
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
