#pragma once

#include "FirstEngine/Device/Export.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Device {
        // Forward declarations
        class ShaderModule;
        class RenderPass;

        // Graphics pipeline description
        FE_DEVICE_API struct GraphicsPipelineDescription {
            // Shader stages
            ShaderModule* vertexShader;
            ShaderModule* fragmentShader;
            ShaderModule* geometryShader = nullptr;
            ShaderModule* tessellationControlShader = nullptr;
            ShaderModule* tessellationEvaluationShader = nullptr;

            // Vertex input
            VkVertexInputBindingDescription vertexBinding;
            std::vector<VkVertexInputAttributeDescription> vertexAttributes;

            // Input assembly
            VkPrimitiveTopology primitiveTopology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            // Viewport and scissor
            VkViewport viewport;
            VkRect2D scissor;

            // Rasterization
            VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
            VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
            VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

            // Multisampling
            VkSampleCountFlagBits rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // Depth and stencil testing
            bool depthTestEnable = true;
            bool depthWriteEnable = true;
            VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS;

            // Color blending
            bool blendEnable = false;
            VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
            VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;

            // Descriptor set layouts
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;

            // Push constant ranges
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

        // Graphics pipeline wrapper
        class FE_DEVICE_API GraphicsPipeline {
        public:
            GraphicsPipeline(DeviceContext* context);
            ~GraphicsPipeline();

            // Create graphics pipeline
            bool Create(RenderPass* renderPass, const GraphicsPipelineDescription& description);

            // Destroy pipeline
            void Destroy();

            VkPipeline GetPipeline() const { return m_Pipeline; }
            VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
            bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

        private:
            void CreatePipelineLayout(const GraphicsPipelineDescription& description);

            DeviceContext* m_Context;
            VkPipeline m_Pipeline;
            VkPipelineLayout m_PipelineLayout;
        };

        // Compute pipeline description
        FE_DEVICE_API struct ComputePipelineDescription {
            ShaderModule* computeShader;
            std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
            std::vector<VkPushConstantRange> pushConstantRanges;
        };

        // Compute pipeline wrapper
        class FE_DEVICE_API ComputePipeline {
        public:
            ComputePipeline(DeviceContext* context);
            ~ComputePipeline();

            // Create compute pipeline
            bool Create(const ComputePipelineDescription& description);

            // Destroy pipeline
            void Destroy();

            VkPipeline GetPipeline() const { return m_Pipeline; }
            VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }
            bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

        private:
            void CreatePipelineLayout(const ComputePipelineDescription& description);

            DeviceContext* m_Context;
            VkPipeline m_Pipeline;
            VkPipelineLayout m_PipelineLayout;
        };

    } // namespace Device
} // namespace FirstEngine
