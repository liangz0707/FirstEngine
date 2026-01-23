// Include standard library headers first, before any namespace definitions
#include <algorithm>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

// Then include project headers
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/FrameGraph.h"

namespace FirstEngine {
    namespace Renderer {

        FrameGraphExecutionPlan::FrameGraphExecutionPlan() = default;
        FrameGraphExecutionPlan::~FrameGraphExecutionPlan() = default;

        void FrameGraphExecutionPlan::AddNodePlan(const NodePlan& nodePlan) {
            if (m_NodeNameToIndex.find(nodePlan.name) != m_NodeNameToIndex.end()) {
                return; // Node already exists
            }

            uint32_t index = static_cast<uint32_t>(m_NodePlans.size());
            m_NodePlans.push_back(nodePlan);
            m_NodePlans.back().index = index;
            m_NodeNameToIndex[nodePlan.name] = index;
        }

        void FrameGraphExecutionPlan::AddNodePlan(NodePlan&& nodePlan) {
            if (m_NodeNameToIndex.find(nodePlan.name) != m_NodeNameToIndex.end()) {
                return; // Node already exists
            }

            uint32_t index = static_cast<uint32_t>(m_NodePlans.size());
            nodePlan.index = index;
            m_NodePlans.push_back(std::move(nodePlan));
            m_NodeNameToIndex[m_NodePlans.back().name] = index;
        }

        // ResourcePlan copy constructor
        FrameGraphExecutionPlan::ResourcePlan::ResourcePlan(const ResourcePlan& other)
            : name(other.name), type(other.type) {
            // Clone the ResourceDescription
            if (other.description) {
                // We need to clone based on type
                if (other.type == ResourceType::Attachment) {
                    auto* attachment = dynamic_cast<const AttachmentResource*>(other.description.get());
                    if (attachment) {
                        description = std::make_unique<AttachmentResource>(
                            attachment->GetName(),
                            attachment->GetWidth(),
                            attachment->GetHeight(),
                            attachment->GetFormat(),
                            attachment->HasDepth()
                        );
                    }
                } else if (other.type == ResourceType::Buffer) {
                    auto* buffer = dynamic_cast<const BufferResource*>(other.description.get());
                    if (buffer) {
                        description = std::make_unique<BufferResource>(
                            buffer->GetName(),
                            buffer->GetSize(),
                            buffer->GetBufferUsage()
                        );
                    }
                } else {
                    // Fallback: create a basic ResourceDescription
                    description = std::make_unique<ResourceDescription>(other.type, other.name);
                }
            }
        }

        // ResourcePlan copy assignment
        FrameGraphExecutionPlan::ResourcePlan& FrameGraphExecutionPlan::ResourcePlan::operator=(const ResourcePlan& other) {
            if (this != &other) {
                name = other.name;
                type = other.type;
                // Clone the ResourceDescription
                if (other.description) {
                    if (other.type == ResourceType::Attachment) {
                        auto* attachment = dynamic_cast<const AttachmentResource*>(other.description.get());
                        if (attachment) {
                            description = std::make_unique<AttachmentResource>(
                                attachment->GetName(),
                                attachment->GetWidth(),
                                attachment->GetHeight(),
                                attachment->GetFormat(),
                                attachment->HasDepth()
                            );
                        }
                    } else if (other.type == ResourceType::Buffer) {
                        auto* buffer = dynamic_cast<const BufferResource*>(other.description.get());
                        if (buffer) {
                            description = std::make_unique<BufferResource>(
                                buffer->GetName(),
                                buffer->GetSize(),
                                buffer->GetBufferUsage()
                            );
                        }
                    } else {
                        description = std::make_unique<ResourceDescription>(other.type, other.name);
                    }
                } else {
                    description.reset();
                }
            }
            return *this;
        }

        void FrameGraphExecutionPlan::AddResourcePlan(const ResourcePlan& resourcePlan) {
            if (m_ResourceNameToIndex.find(resourcePlan.name) != m_ResourceNameToIndex.end()) {
                return; // Resource already exists
            }

            uint32_t index = static_cast<uint32_t>(m_ResourcePlans.size());
            m_ResourcePlans.push_back(resourcePlan);
            m_ResourceNameToIndex[resourcePlan.name] = index;
        }

        void FrameGraphExecutionPlan::AddResourcePlan(ResourcePlan&& resourcePlan) {
            if (m_ResourceNameToIndex.find(resourcePlan.name) != m_ResourceNameToIndex.end()) {
                return; // Resource already exists
            }

            uint32_t index = static_cast<uint32_t>(m_ResourcePlans.size());
            m_ResourcePlans.push_back(std::move(resourcePlan));
            m_ResourceNameToIndex[m_ResourcePlans.back().name] = index;
        }

        const FrameGraphExecutionPlan::NodePlan* FrameGraphExecutionPlan::GetNodePlan(uint32_t index) const {
            if (index < m_NodePlans.size()) {
                return &m_NodePlans[index];
            }
            return nullptr;
        }

        const FrameGraphExecutionPlan::NodePlan* FrameGraphExecutionPlan::GetNodePlan(const std::string& name) const {
            auto it = m_NodeNameToIndex.find(name);
            if (it != m_NodeNameToIndex.end()) {
                return &m_NodePlans[it->second];
            }
            return nullptr;
        }

        const FrameGraphExecutionPlan::ResourcePlan* FrameGraphExecutionPlan::GetResourcePlan(const std::string& name) const {
            auto it = m_ResourceNameToIndex.find(name);
            if (it != m_ResourceNameToIndex.end()) {
                return &m_ResourcePlans[it->second];
            }
            return nullptr;
        }

        void FrameGraphExecutionPlan::Clear() {
            m_NodePlans.clear();
            m_ResourcePlans.clear();
            m_ExecutionOrder.clear();
            m_NodeNameToIndex.clear();
            m_ResourceNameToIndex.clear();
        }

        bool FrameGraphExecutionPlan::IsValid() const {
            // Check if execution order matches node count
            if (m_ExecutionOrder.size() != m_NodePlans.size()) {
                return false;
            }

            // Check if all nodes in execution order are valid
            for (uint32_t index : m_ExecutionOrder) {
                if (index >= m_NodePlans.size()) {
                    return false;
                }
            }

            // Check if all dependencies are valid
            for (const auto& nodePlan : m_NodePlans) {
                for (uint32_t dep : nodePlan.dependencies) {
                    if (dep >= m_NodePlans.size()) {
                        return false;
                    }
                }
            }

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
