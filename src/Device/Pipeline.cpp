#include "FirstEngine/Device/Pipeline.h"
#include "FirstEngine/Device/DeviceContext.h"
#include "FirstEngine/Device/RenderPass.h"
#include "FirstEngine/Device/ShaderModule.h"
#include <stdexcept>

namespace FirstEngine {
    namespace Device {

        // GraphicsPipeline 实现
        GraphicsPipeline::GraphicsPipeline(DeviceContext* context)
            : m_Context(context), m_Pipeline(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE) {
        }

        GraphicsPipeline::~GraphicsPipeline() {
            Destroy();
        }

        bool GraphicsPipeline::Create(RenderPass* renderPass, const GraphicsPipelineDescription& description) {
            if (!description.vertexShader || !description.fragmentShader) {
                return false;
            }

            CreatePipelineLayout(description);

            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

            // 顶点着色器
            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = description.vertexShader->GetShaderModule();
            vertShaderStageInfo.pName = "main";
            shaderStages.push_back(vertShaderStageInfo);

            // 片段着色器
            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = description.fragmentShader->GetShaderModule();
            fragShaderStageInfo.pName = "main";
            shaderStages.push_back(fragShaderStageInfo);

            // 顶点输入
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &description.vertexBinding;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(description.vertexAttributes.size());
            vertexInputInfo.pVertexAttributeDescriptions = description.vertexAttributes.data();

            // 输入装配
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = description.primitiveTopology;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            // 视口和裁剪
            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &description.viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &description.scissor;

            // 光栅化
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = description.polygonMode;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = description.cullMode;
            rasterizer.frontFace = description.frontFace;
            rasterizer.depthBiasEnable = VK_FALSE;

            // 多重采样
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = description.rasterizationSamples;

            // 深度和模板测试
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = description.depthTestEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthWriteEnable = description.depthWriteEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = description.depthCompareOp;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;

            // 颜色混合
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = description.blendEnable ? VK_TRUE : VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = description.srcColorBlendFactor;
            colorBlendAttachment.dstColorBlendFactor = description.dstColorBlendFactor;
            colorBlendAttachment.colorBlendOp = description.colorBlendOp;
            colorBlendAttachment.srcAlphaBlendFactor = description.srcAlphaBlendFactor;
            colorBlendAttachment.dstAlphaBlendFactor = description.dstAlphaBlendFactor;
            colorBlendAttachment.alphaBlendOp = description.alphaBlendOp;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;

            // 动态状态
            VkDynamicState dynamicStates[] = {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR
            };

            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamicStates;

            // 创建图形管道
            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
            pipelineInfo.pStages = shaderStages.data();
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = m_PipelineLayout;
            pipelineInfo.renderPass = renderPass->GetRenderPass();
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            if (vkCreateGraphicsPipelines(m_Context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        void GraphicsPipeline::Destroy() {
            if (m_Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Context->GetDevice(), m_Pipeline, nullptr);
                m_Pipeline = VK_NULL_HANDLE;
            }

            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Context->GetDevice(), m_PipelineLayout, nullptr);
                m_PipelineLayout = VK_NULL_HANDLE;
            }
        }

        void GraphicsPipeline::CreatePipelineLayout(const GraphicsPipelineDescription& description) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(description.descriptorSetLayouts.size());
            pipelineLayoutInfo.pSetLayouts = description.descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(description.pushConstantRanges.size());
            pipelineLayoutInfo.pPushConstantRanges = description.pushConstantRanges.data();

            if (vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create pipeline layout!");
            }
        }

        // ComputePipeline 实现
        ComputePipeline::ComputePipeline(DeviceContext* context)
            : m_Context(context), m_Pipeline(VK_NULL_HANDLE), m_PipelineLayout(VK_NULL_HANDLE) {
        }

        ComputePipeline::~ComputePipeline() {
            Destroy();
        }

        bool ComputePipeline::Create(const ComputePipelineDescription& description) {
            if (!description.computeShader) {
                return false;
            }

            CreatePipelineLayout(description);

            VkPipelineShaderStageCreateInfo shaderStageInfo{};
            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderStageInfo.module = description.computeShader->GetShaderModule();
            shaderStageInfo.pName = "main";

            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.stage = shaderStageInfo;
            pipelineInfo.layout = m_PipelineLayout;

            if (vkCreateComputePipelines(m_Context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
                return false;
            }

            return true;
        }

        void ComputePipeline::Destroy() {
            if (m_Pipeline != VK_NULL_HANDLE) {
                vkDestroyPipeline(m_Context->GetDevice(), m_Pipeline, nullptr);
                m_Pipeline = VK_NULL_HANDLE;
            }

            if (m_PipelineLayout != VK_NULL_HANDLE) {
                vkDestroyPipelineLayout(m_Context->GetDevice(), m_PipelineLayout, nullptr);
                m_PipelineLayout = VK_NULL_HANDLE;
            }
        }

        void ComputePipeline::CreatePipelineLayout(const ComputePipelineDescription& description) {
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(description.descriptorSetLayouts.size());
            pipelineLayoutInfo.pSetLayouts = description.descriptorSetLayouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(description.pushConstantRanges.size());
            pipelineLayoutInfo.pPushConstantRanges = description.pushConstantRanges.data();

            if (vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create compute pipeline layout!");
            }
        }

    } // namespace Device
} // namespace FirstEngine
