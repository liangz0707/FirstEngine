#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace FirstEngine {
    namespace Renderer {
        class VulkanRenderer;

        class FE_RENDERER_API Pipeline {
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

            VkShaderModule CreateShaderModule(const std::vector<char>& code);

            VulkanRenderer* m_Renderer;
            VkRenderPass m_RenderPass;
            VkPipelineLayout m_PipelineLayout;
            VkPipeline m_GraphicsPipeline;
            std::vector<VkFramebuffer> m_SwapchainFramebuffers;
        };
    }
}
