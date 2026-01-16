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

        // FrameGraph 执行计划 - 与 CommandBuffer 和 Device 无关的中间结构
        // 这个结构描述了渲染流程，但不包含实际的 GPU 资源或命令
        class FE_RENDERER_API FrameGraphExecutionPlan {
        public:
            FrameGraphExecutionPlan();
            ~FrameGraphExecutionPlan();

            // 执行计划中的节点信息
            struct NodePlan {
                std::string name;
                uint32_t index;
                RenderPassType type; // Pass type using enum
                std::string typeString; // String representation (for serialization/debugging)
                std::vector<std::string> readResources;
                std::vector<std::string> writeResources;
                std::vector<uint32_t> dependencies; // 依赖的节点索引
            };

            // 执行计划中的资源信息
            struct ResourcePlan {
                std::string name;
                ResourceType type;
                std::unique_ptr<ResourceDescription> description; // Use pointer to support polymorphism
                // 不包含实际的 RHI 资源指针，只有描述信息
                
                ResourcePlan() = default;
                ResourcePlan(const ResourcePlan& other);
                ResourcePlan(ResourcePlan&& other) noexcept = default;
                ResourcePlan& operator=(const ResourcePlan& other);
                ResourcePlan& operator=(ResourcePlan&& other) noexcept = default;
            };

            // 添加节点计划
            void AddNodePlan(const NodePlan& nodePlan);
            void AddNodePlan(NodePlan&& nodePlan);

            // 添加资源计划
            void AddResourcePlan(const ResourcePlan& resourcePlan);
            void AddResourcePlan(ResourcePlan&& resourcePlan);

            // 获取执行顺序（拓扑排序后的节点索引）
            const std::vector<uint32_t>& GetExecutionOrder() const { return m_ExecutionOrder; }
            void SetExecutionOrder(const std::vector<uint32_t>& order) { m_ExecutionOrder = order; }

            // 获取节点计划
            const std::vector<NodePlan>& GetNodePlans() const { return m_NodePlans; }
            const NodePlan* GetNodePlan(uint32_t index) const;
            const NodePlan* GetNodePlan(const std::string& name) const;

            // 获取资源计划
            const std::vector<ResourcePlan>& GetResourcePlans() const { return m_ResourcePlans; }
            const ResourcePlan* GetResourcePlan(const std::string& name) const;

            // 清空计划
            void Clear();

            // 检查计划是否有效
            bool IsValid() const;

        private:
            std::vector<NodePlan> m_NodePlans;
            std::vector<ResourcePlan> m_ResourcePlans;
            std::vector<uint32_t> m_ExecutionOrder; // 拓扑排序后的执行顺序
            std::unordered_map<std::string, uint32_t> m_NodeNameToIndex;
            std::unordered_map<std::string, uint32_t> m_ResourceNameToIndex;
        };

    } // namespace Renderer
} // namespace FirstEngine
