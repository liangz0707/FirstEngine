#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/ResourceTypes.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

// Forward declarations
namespace FirstEngine {
    namespace Renderer {
        class ResourceDescription;
        class AttachmentResource;
        class BufferResource;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // FrameGraph execution plan - Intermediate structure independent of CommandBuffer and Device
        // This structure describes the rendering flow but does not contain actual GPU resources or commands
        class FE_RENDERER_API FrameGraphExecutionPlan {
        public:
            FrameGraphExecutionPlan();
            ~FrameGraphExecutionPlan();

            // Node information in execution plan
            struct NodePlan {
                std::string name;
                uint32_t index;
                RenderPassType type; // Pass type using enum
                std::string typeString; // String representation (for serialization/debugging)
                std::vector<std::string> readResources;
                std::vector<std::string> writeResources;
                std::vector<uint32_t> dependencies; // Dependent node indices
            };

            // Resource information in execution plan
            struct ResourcePlan {
                std::string name;
                ResourceType type;
                std::unique_ptr<ResourceDescription> description; // Use pointer to support polymorphism
                // Does not contain actual RHI resource pointers, only description information
                
                ResourcePlan() = default;
                ResourcePlan(const ResourcePlan& other);
                ResourcePlan(ResourcePlan&& other) noexcept = default;
                ResourcePlan& operator=(const ResourcePlan& other);
                ResourcePlan& operator=(ResourcePlan&& other) noexcept = default;
            };

            // Add node plan
            void AddNodePlan(const NodePlan& nodePlan);
            void AddNodePlan(NodePlan&& nodePlan);

            // Add resource plan
            void AddResourcePlan(const ResourcePlan& resourcePlan);
            void AddResourcePlan(ResourcePlan&& resourcePlan);

            // Get execution order (topologically sorted node indices)
            const std::vector<uint32_t>& GetExecutionOrder() const { return m_ExecutionOrder; }
            void SetExecutionOrder(const std::vector<uint32_t>& order) { m_ExecutionOrder = order; }

            // Get node plan
            const std::vector<NodePlan>& GetNodePlans() const { return m_NodePlans; }
            const NodePlan* GetNodePlan(uint32_t index) const;
            const NodePlan* GetNodePlan(const std::string& name) const;

            // Get resource plan
            const std::vector<ResourcePlan>& GetResourcePlans() const { return m_ResourcePlans; }
            const ResourcePlan* GetResourcePlan(const std::string& name) const;

            // Clear plan
            void Clear();

            // Check if plan is valid
            bool IsValid() const;

        private:
            std::vector<NodePlan> m_NodePlans;
            std::vector<ResourcePlan> m_ResourcePlans;
            std::vector<uint32_t> m_ExecutionOrder; // Execution order after topological sort
            std::unordered_map<std::string, uint32_t> m_NodeNameToIndex;
            std::unordered_map<std::string, uint32_t> m_ResourceNameToIndex;
        };

    } // namespace Renderer
} // namespace FirstEngine
