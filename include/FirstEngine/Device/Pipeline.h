#pragma once

#include "FirstEngine/Device/Export.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace FirstEngine {
    namespace Device {
        class VulkanRenderer;

        class FE_DEVICE_API Pipeline {
        public:
            Pipeline(VulkanRenderer* renderer);
            ~Pipeline();

            VkPipeline GetPipeline() const { return m_GraphicsPipeline; }
            VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
            VkRenderPass GetRenderPass() const { return m_RenderPass; }

        private:
            void CreateRenderPass();
            void CreateGraphicsPipeline();
            void CreateFramebuffers();

            VulkanRenderer* m_Renderer;
            VkRenderPass m_RenderPass;
            VkPipelineLayout m_PipelineLayout;
            VkPipeline m_GraphicsPipeline;
            std::vector<VkFramebuffer> m_SwapchainFramebuffers;
        };
    }
}
