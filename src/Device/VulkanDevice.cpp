#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Device/VulkanRHIWrappers.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Device/DeviceContext.h"
#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Device/ShaderModule.h"  // Still needed for VulkanShaderModule wrapper
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <iostream>
#include <stdexcept>
#include <algorithm>

#ifdef _WIN32
// Undefine Windows API macros that conflict with our method names
// Must be after all includes that might include Windows.h
#ifdef CreateMutex
#undef CreateMutex
#endif
#ifdef CreateEvent
#undef CreateEvent
#endif
#endif

namespace FirstEngine {
    namespace Device {

        VulkanDevice::VulkanDevice() : m_DefaultSampler(VK_NULL_HANDLE) {
        }

        VulkanDevice::~VulkanDevice() {
            Shutdown();
        }

        bool VulkanDevice::Initialize(void* windowHandle) {
            // windowHandle should be GLFWwindow*
            GLFWwindow* glfwWindow = static_cast<GLFWwindow*>(windowHandle);
            if (!glfwWindow) {
                return false;
            }

            // Create Window wrapper from existing GLFWwindow
            // This avoids re-initializing GLFW since the window was already created by Application
            try {
                m_Window = std::make_unique<Core::Window>(glfwWindow);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create Window wrapper: " << e.what() << std::endl;
                return false;
            }

            // Create VulkanRenderer
            m_Renderer = std::make_unique<VulkanRenderer>(m_Window.get());

            // Fill device information
            m_DeviceInfo.deviceName = "Vulkan Device";
            m_DeviceInfo.apiVersion = VK_API_VERSION_1_0;
            m_DeviceInfo.driverVersion = 0;
            m_DeviceInfo.deviceMemory = 0; // Can be obtained from physical device
            m_DeviceInfo.hostMemory = 0;

            return true;
        }

        void VulkanDevice::Shutdown() {
            // Destroy default sampler if created
            if (m_DefaultSampler != VK_NULL_HANDLE && m_Renderer) {
                auto* context = m_Renderer->GetDeviceContext();
                if (context) {
                    vkDestroySampler(context->GetDevice(), m_DefaultSampler, nullptr);
                    m_DefaultSampler = VK_NULL_HANDLE;
                }
            }
            m_Renderer.reset();
            m_Window.reset();
        }

        std::unique_ptr<RHI::ICommandBuffer> VulkanDevice::CreateCommandBuffer() {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }
            try {
                return std::make_unique<VulkanCommandBuffer>(const_cast<DeviceContext*>(context));
            } catch (...) {
                return nullptr;
            }
        }

        std::unique_ptr<RHI::IRenderPass> VulkanDevice::CreateRenderPass(const RHI::RenderPassDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            // Limit color attachment count (Vulkan spec requires max 8)
            const uint32_t maxColorAttachments = 8;
            uint32_t colorAttachmentCount = static_cast<uint32_t>(desc.colorAttachments.size());
            if (colorAttachmentCount > maxColorAttachments) {
                std::cerr << "Warning: Color attachment count (" << colorAttachmentCount 
                          << ") exceeds maximum (" << maxColorAttachments 
                          << "). Clamping to maximum." << std::endl;
                colorAttachmentCount = maxColorAttachments;
            }

            // Convert attachment descriptions
            std::vector<VkAttachmentDescription> attachments;
            for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
                const auto& colorAttach = desc.colorAttachments[i];
                VkAttachmentDescription attachment{};
                attachment.format = ConvertFormat(colorAttach.format);
                attachment.samples = static_cast<VkSampleCountFlagBits>(colorAttach.samples);
                attachment.loadOp = colorAttach.loadOpClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.storeOp = colorAttach.storeOpStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                attachment.stencilLoadOp = colorAttach.stencilLoadOpClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                attachment.stencilStoreOp = colorAttach.stencilStoreOpStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                
                // initialLayout: If undefined, use UNDEFINED (allowed)
                VkImageLayout initialLayout = ConvertImageLayout(colorAttach.initialLayout);
                if (initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && colorAttach.initialLayout != RHI::Format::Undefined) {
                    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Keep UNDEFINED
                }
                attachment.initialLayout = initialLayout;
                
                // finalLayout: Cannot be UNDEFINED, use default COLOR_ATTACHMENT_OPTIMAL
                VkImageLayout finalLayout = ConvertImageLayout(colorAttach.finalLayout);
                if (finalLayout == VK_IMAGE_LAYOUT_UNDEFINED || colorAttach.finalLayout == RHI::Format::Undefined) {
                    finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
                attachment.finalLayout = finalLayout;
                
                attachments.push_back(attachment);
            }

            if (desc.hasDepthAttachment) {
                VkAttachmentDescription depthAttachment{};
                depthAttachment.format = ConvertFormat(desc.depthAttachment.format);
                depthAttachment.samples = static_cast<VkSampleCountFlagBits>(desc.depthAttachment.samples);
                depthAttachment.loadOp = desc.depthAttachment.loadOpClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                depthAttachment.storeOp = desc.depthAttachment.storeOpStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                depthAttachment.stencilLoadOp = desc.depthAttachment.stencilLoadOpClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
                depthAttachment.stencilStoreOp = desc.depthAttachment.stencilStoreOpStore ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
                
                // initialLayout: If undefined, use UNDEFINED (allowed)
                VkImageLayout initialLayout = ConvertImageLayout(desc.depthAttachment.initialLayout);
                if (initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && desc.depthAttachment.initialLayout != RHI::Format::Undefined) {
                    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Keep UNDEFINED
                }
                depthAttachment.initialLayout = initialLayout;
                
                // finalLayout: Cannot be UNDEFINED, use default DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                VkImageLayout finalLayout = ConvertImageLayout(desc.depthAttachment.finalLayout);
                if (finalLayout == VK_IMAGE_LAYOUT_UNDEFINED || desc.depthAttachment.finalLayout == RHI::Format::Undefined) {
                    finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                depthAttachment.finalLayout = finalLayout;
                
                attachments.push_back(depthAttachment);
            }

            // Create subpass
            std::vector<VkAttachmentReference> colorRefs;
            for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
                VkAttachmentReference ref{};
                ref.attachment = i;
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorRefs.push_back(ref);
            }

            VkAttachmentReference depthRef{};
            if (desc.hasDepthAttachment) {
                depthRef.attachment = colorAttachmentCount; // Use clamped color attachment count
                depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            }

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
            subpass.pColorAttachments = colorRefs.empty() ? nullptr : colorRefs.data();
            if (desc.hasDepthAttachment) {
                subpass.pDepthStencilAttachment = &depthRef;
            } else {
                subpass.pDepthStencilAttachment = nullptr;
            }

            // Subpass dependencies
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency.dependencyFlags = 0;

            // Create render pass
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.pNext = nullptr;
            renderPassInfo.flags = 0;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.empty() ? nullptr : attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            VkRenderPass renderPass = VK_NULL_HANDLE;
            VkResult result = vkCreateRenderPass(context->GetDevice(), &renderPassInfo, nullptr, &renderPass);
            if (result != VK_SUCCESS) {
                std::cerr << "Failed to create render pass: VkResult = " << result << std::endl;
                return nullptr;
            }

            // Store color attachment count for pipeline creation
            uint32_t storedColorAttachmentCount = colorAttachmentCount;
            return std::make_unique<VulkanRenderPass>(context, renderPass, storedColorAttachmentCount);
        }

        std::unique_ptr<RHI::IFramebuffer> VulkanDevice::CreateFramebuffer(
            RHI::IRenderPass* renderPass,
            const std::vector<RHI::IImageView*>& attachments,
            uint32_t width, uint32_t height) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context || !renderPass) {
                return nullptr;
            }

            auto* vkRenderPass = static_cast<VulkanRenderPass*>(renderPass);
            std::vector<VkImageView> vkAttachments;
            for (auto* attachment : attachments) {
                auto* vkImageView = static_cast<VulkanImageView*>(attachment);
                vkAttachments.push_back(vkImageView->GetVkImageView());
            }

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = vkRenderPass->GetVkRenderPass();
            framebufferInfo.attachmentCount = static_cast<uint32_t>(vkAttachments.size());
            framebufferInfo.pAttachments = vkAttachments.data();
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            VkFramebuffer framebuffer;
            if (vkCreateFramebuffer(context->GetDevice(), &framebufferInfo, nullptr, &framebuffer) != VK_SUCCESS) {
                return nullptr;
            }

            return std::make_unique<VulkanFramebuffer>(context, framebuffer, width, height);
        }

        std::unique_ptr<RHI::IPipeline> VulkanDevice::CreateGraphicsPipeline(
            const RHI::GraphicsPipelineDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context || !desc.renderPass || desc.shaderModules.empty()) {
                return nullptr;
            }

            auto* vkRenderPass = static_cast<VulkanRenderPass*>(desc.renderPass);

            // Create shader stages
            std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
            for (auto* shaderModule : desc.shaderModules) {
                auto* vkShader = static_cast<VulkanShaderModule*>(shaderModule);
                VkPipelineShaderStageCreateInfo stageInfo{};
                stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stageInfo.stage = ConvertShaderStage(vkShader->GetStage());
                stageInfo.module = vkShader->GetVkShaderModule();
                stageInfo.pName = "main";
                shaderStages.push_back(stageInfo);
            }

            // Vertex input
            std::vector<VkVertexInputBindingDescription> bindings;
            for (const auto& binding : desc.vertexBindings) {
                VkVertexInputBindingDescription vkBinding{};
                vkBinding.binding = binding.binding;
                vkBinding.stride = binding.stride;
                vkBinding.inputRate = binding.instanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
                bindings.push_back(vkBinding);
            }

            std::vector<VkVertexInputAttributeDescription> attributes;
            for (const auto& attr : desc.vertexAttributes) {
                VkVertexInputAttributeDescription vkAttr{};
                vkAttr.location = attr.location;
                vkAttr.binding = attr.binding;
                vkAttr.format = ConvertFormat(attr.format);
                vkAttr.offset = attr.offset;
                attributes.push_back(vkAttr);
            }

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
            vertexInputInfo.pVertexBindingDescriptions = bindings.data();
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
            vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

            // Input assembly
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = ConvertPrimitiveTopology(desc.primitiveTopology);
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            // Viewport and scissor
            VkViewport viewport{};
            viewport.x = desc.viewport.x;
            viewport.y = desc.viewport.y;
            viewport.width = desc.viewport.width;
            viewport.height = desc.viewport.height;
            viewport.minDepth = desc.viewport.minDepth;
            viewport.maxDepth = desc.viewport.maxDepth;

            VkRect2D scissor{};
            scissor.offset = {desc.scissor.x, desc.scissor.y};
            scissor.extent = {desc.scissor.width, desc.scissor.height};

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = desc.rasterizationState.depthClampEnable ? VK_TRUE : VK_FALSE;
            rasterizer.rasterizerDiscardEnable = desc.rasterizationState.rasterizerDiscardEnable ? VK_TRUE : VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = desc.rasterizationState.lineWidth;
            rasterizer.cullMode = ConvertCullMode(desc.rasterizationState.cullMode);
            rasterizer.frontFace = desc.rasterizationState.frontFaceCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = desc.rasterizationState.depthBiasEnable ? VK_TRUE : VK_FALSE;
            rasterizer.depthBiasConstantFactor = desc.rasterizationState.depthBiasConstantFactor;
            rasterizer.depthBiasClamp = desc.rasterizationState.depthBiasClamp;
            rasterizer.depthBiasSlopeFactor = desc.rasterizationState.depthBiasSlopeFactor;

            // Multisampling
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // Depth stencil
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = desc.depthStencilState.depthTestEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthWriteEnable = desc.depthStencilState.depthWriteEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = ConvertCompareOp(desc.depthStencilState.depthCompareOp);
            depthStencil.depthBoundsTestEnable = desc.depthStencilState.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
            depthStencil.stencilTestEnable = desc.depthStencilState.stencilTestEnable ? VK_TRUE : VK_FALSE;

            // Color blending
            // Get color attachment count from render pass
            uint32_t requiredColorAttachmentCount = vkRenderPass->GetColorAttachmentCount();
            
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
            
            // If colorBlendAttachments is provided, use them (but ensure count matches)
            if (!desc.colorBlendAttachments.empty()) {
                // Use provided attachments, but ensure we have enough for all color attachments
                for (const auto& attachment : desc.colorBlendAttachments) {
                    VkPipelineColorBlendAttachmentState blendAttachment{};
                    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                    blendAttachment.blendEnable = attachment.blendEnable ? VK_TRUE : VK_FALSE;
                    blendAttachment.srcColorBlendFactor = static_cast<VkBlendFactor>(attachment.srcColorBlendFactor);
                    blendAttachment.dstColorBlendFactor = static_cast<VkBlendFactor>(attachment.dstColorBlendFactor);
                    blendAttachment.colorBlendOp = static_cast<VkBlendOp>(attachment.colorBlendOp);
                    blendAttachment.srcAlphaBlendFactor = static_cast<VkBlendFactor>(attachment.srcAlphaBlendFactor);
                    blendAttachment.dstAlphaBlendFactor = static_cast<VkBlendFactor>(attachment.dstAlphaBlendFactor);
                    blendAttachment.alphaBlendOp = static_cast<VkBlendOp>(attachment.alphaBlendOp);
                    colorBlendAttachments.push_back(blendAttachment);
                }
                
                // If we have fewer attachments than required, add default ones
                while (colorBlendAttachments.size() < requiredColorAttachmentCount) {
                    VkPipelineColorBlendAttachmentState blendAttachment{};
                    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                    blendAttachment.blendEnable = VK_FALSE;
                    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                    colorBlendAttachments.push_back(blendAttachment);
                }
            } else {
                // No color blend attachments provided - create default ones for all color attachments
                for (uint32_t i = 0; i < requiredColorAttachmentCount; ++i) {
                    VkPipelineColorBlendAttachmentState blendAttachment{};
                    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                    blendAttachment.blendEnable = VK_FALSE;
                    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
                    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
                    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
                    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
                    colorBlendAttachments.push_back(blendAttachment);
                }
            }

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
            colorBlending.pAttachments = colorBlendAttachments.data();
            
            // Validate that attachment count matches render pass
            if (colorBlending.attachmentCount != requiredColorAttachmentCount) {
                std::cerr << "Error: VulkanDevice::CreateGraphicsPipeline: Color blend attachment count ("
                          << colorBlending.attachmentCount << ") does not match render pass color attachment count ("
                          << requiredColorAttachmentCount << ")" << std::endl;
                return nullptr;
            }

            // Dynamic state
            VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamicStates;

            // Pipeline layout
            std::vector<VkPushConstantRange> pushConstantRanges;
            for (const auto& range : desc.pushConstantRanges) {
                VkPushConstantRange vkRange{};
                vkRange.offset = range.offset;
                vkRange.size = range.size;
                vkRange.stageFlags = ConvertShaderStage(range.stageFlags);
                pushConstantRanges.push_back(vkRange);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desc.descriptorSetLayouts.size());
            pipelineLayoutInfo.pSetLayouts = reinterpret_cast<const VkDescriptorSetLayout*>(desc.descriptorSetLayouts.data());
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

            VkPipelineLayout pipelineLayout;
            if (vkCreatePipelineLayout(context->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                return nullptr;
            }

            // Create graphics pipeline
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
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.renderPass = vkRenderPass->GetVkRenderPass();
            pipelineInfo.subpass = 0;

            VkPipeline pipeline;
            if (vkCreateGraphicsPipelines(context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                vkDestroyPipelineLayout(context->GetDevice(), pipelineLayout, nullptr);
                return nullptr;
            }

            return std::make_unique<VulkanPipeline>(context, pipeline, pipelineLayout);
        }

        std::unique_ptr<RHI::IPipeline> VulkanDevice::CreateComputePipeline(
            const RHI::ComputePipelineDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            if (!desc.computeShader) {
                return nullptr;
            }

            // Get shader module
            auto* vkShader = static_cast<VulkanShaderModule*>(desc.computeShader);
            if (!vkShader) {
                return nullptr;
            }

            // Create pipeline layout
            std::vector<VkPushConstantRange> pushConstantRanges;
            for (const auto& range : desc.pushConstantRanges) {
                VkPushConstantRange vkRange{};
                vkRange.offset = range.offset;
                vkRange.size = range.size;
                vkRange.stageFlags = ConvertShaderStage(range.stageFlags);
                pushConstantRanges.push_back(vkRange);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(desc.descriptorSetLayouts.size());
            pipelineLayoutInfo.pSetLayouts = reinterpret_cast<const VkDescriptorSetLayout*>(desc.descriptorSetLayouts.data());
            pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

            VkPipelineLayout pipelineLayout;
            if (vkCreatePipelineLayout(context->GetDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                return nullptr;
            }

            // Create compute pipeline
            VkPipelineShaderStageCreateInfo shaderStageInfo{};
            shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            shaderStageInfo.module = vkShader->GetVkShaderModule();
            shaderStageInfo.pName = "main";

            VkComputePipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
            pipelineInfo.stage = shaderStageInfo;
            pipelineInfo.layout = pipelineLayout;

            VkPipeline pipeline;
            if (vkCreateComputePipelines(context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS) {
                vkDestroyPipelineLayout(context->GetDevice(), pipelineLayout, nullptr);
                return nullptr;
            }

            return std::make_unique<VulkanPipeline>(context, pipeline, pipelineLayout);
        }

        std::unique_ptr<RHI::IBuffer> VulkanDevice::CreateBuffer(
            uint64_t size, RHI::BufferUsageFlags usage, RHI::MemoryPropertyFlags properties) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            MemoryManager memoryManager(context);
            auto buffer = memoryManager.CreateBuffer(
                size,
                ConvertBufferUsage(usage),
                ConvertMemoryProperties(properties)
            );

            if (!buffer) {
                return nullptr;
            }

            return std::make_unique<VulkanBuffer>(context, buffer.release());
        }

        std::unique_ptr<RHI::IImage> VulkanDevice::CreateImage(const RHI::ImageDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            VkFormat vkFormat = ConvertFormat(desc.format);
            
            // For depth formats, ensure usage flags don't include COLOR_ATTACHMENT_BIT
            // and do include DEPTH_STENCIL_ATTACHMENT_BIT
            RHI::ImageUsageFlags correctedUsage = desc.usage;
            bool isDepthFormat = (desc.format == RHI::Format::D32_SFLOAT || desc.format == RHI::Format::D24_UNORM_S8_UINT);
            
            if (isDepthFormat) {
                // Remove COLOR_ATTACHMENT_BIT if present (depth formats can't be color attachments)
                correctedUsage = static_cast<RHI::ImageUsageFlags>(
                    static_cast<uint32_t>(correctedUsage) & ~static_cast<uint32_t>(RHI::ImageUsageFlags::ColorAttachment)
                );
                // Ensure DEPTH_STENCIL_ATTACHMENT_BIT is set
                correctedUsage = static_cast<RHI::ImageUsageFlags>(
                    static_cast<uint32_t>(correctedUsage) | static_cast<uint32_t>(RHI::ImageUsageFlags::DepthStencilAttachment)
                );
            }

            MemoryManager memoryManager(context);
            auto image = memoryManager.CreateImage(
                desc.width,
                desc.height,
                desc.mipLevels,
                VK_SAMPLE_COUNT_1_BIT,
                vkFormat,
                VK_IMAGE_TILING_OPTIMAL,
                ConvertImageUsage(correctedUsage),
                ConvertMemoryProperties(desc.memoryProperties)
            );

            if (!image) {
                return nullptr;
            }

            // Determine aspect flags based on format, not just usage
            // Depth formats must use VK_IMAGE_ASPECT_DEPTH_BIT
            VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
            
            // Check if format is a depth or depth-stencil format
            if (isDepthFormat) {
                if (desc.format == RHI::Format::D24_UNORM_S8_UINT) {
                    // Depth-stencil format needs both aspects
                    aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
                } else {
                    // Depth-only format (D32_SFLOAT)
                    aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
                }
            } else if (static_cast<uint32_t>(desc.usage) & static_cast<uint32_t>(RHI::ImageUsageFlags::DepthStencilAttachment)) {
                // Fallback: if usage indicates depth but format doesn't match known depth formats,
                // still use depth aspect (might be a custom depth format)
                aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
            }
            
            if (!image->CreateImageView(VK_IMAGE_VIEW_TYPE_2D, vkFormat, aspectFlags)) {
                return nullptr;
            }

            return std::make_unique<VulkanImage>(context, image.release());
        }

        std::unique_ptr<RHI::ISwapchain> VulkanDevice::CreateSwapchain(
            void* windowHandle, const RHI::SwapchainDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context || !m_Renderer) {
                return nullptr;
            }

            VkSurfaceKHR surface = VK_NULL_HANDLE;
            Core::Window* window = nullptr;

            // Get main window for comparison
            Core::Window* mainWindow = m_Renderer->GetWindow();
            if (!mainWindow) {
                std::cerr << "No main window available" << std::endl;
                return nullptr;
            }
            
            GLFWwindow* mainGlfwWindow = mainWindow->GetHandle();
            if (!mainGlfwWindow) {
                std::cerr << "Main window has no GLFW handle" << std::endl;
                return nullptr;
            }
            
            // Check if windowHandle is the main GLFW window (standalone mode)
            // or a different HWND (editor mode child window)
            if (windowHandle) {
                // First, check if it's the main GLFW window (standalone mode)
                GLFWwindow* glfwWindow = reinterpret_cast<GLFWwindow*>(windowHandle);
                if (glfwWindow == mainGlfwWindow) {
                    // This is the main GLFW window (standalone mode)
                    // Use existing surface
                    surface = m_Renderer->GetSurface();
                    window = mainWindow;
                    
                    // Verify surface is valid
                    if (surface == VK_NULL_HANDLE) {
                        std::cerr << "Error: Main window surface is null! Surface may not have been created." << std::endl;
                        return nullptr;
                    }
                    
                    std::cout << "Using main window surface for standalone mode (surface: " << surface << ")" << std::endl;
                } else {
                    // Not the main GLFW window, check if it's a HWND (editor mode)
#ifdef _WIN32
                    HWND hwnd = reinterpret_cast<HWND>(windowHandle);
                    HWND mainHwnd = glfwGetWin32Window(mainGlfwWindow);
                    
                    // Validate if this is a valid HWND by checking if it's different from main window
                    // and if GetWindowThreadProcessId works (only works for valid HWND)
                    DWORD processId = 0;
                    DWORD threadId = GetWindowThreadProcessId(hwnd, &processId);
                    
                    if (threadId != 0 && hwnd != mainHwnd) {
                        // This is a valid HWND and different from main window (editor child window)
                        // Create a new surface for this window
                        VkInstance instance = m_Renderer->GetInstance();
                        if (instance != VK_NULL_HANDLE) {
                            VkWin32SurfaceCreateInfoKHR createInfo{};
                            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
                            createInfo.hwnd = hwnd;
                            createInfo.hinstance = GetModuleHandle(nullptr);
                            
                            // Get function pointer
                            auto vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)
                                vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR");
                            
                            if (vkCreateWin32SurfaceKHR) {
                                // Verify window size before creating surface
                                RECT rect;
                                if (GetClientRect(hwnd, &rect)) {
                                    int winWidth = rect.right - rect.left;
                                    int winHeight = rect.bottom - rect.top;
                                    std::cout << "Creating surface for child window (HWND: " << hwnd 
                                              << "), window size: " << winWidth << "x" << winHeight 
                                              << ", requested: " << desc.width << "x" << desc.height << std::endl;
                                }
                                
                                VkResult result = vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface);
                                if (result != VK_SUCCESS) {
                                    std::cerr << "Failed to create surface for child window: " << result << std::endl;
                                    return nullptr;
                                }
                                std::cout << "Successfully created new surface for child window" << std::endl;
                            } else {
                                std::cerr << "vkCreateWin32SurfaceKHR not available" << std::endl;
                                return nullptr;
                            }
                        }
                    } else {
                        // Invalid HWND or same as main window, use main window surface
                        std::cout << "Using main window surface (windowHandle is main window or invalid)" << std::endl;
                        surface = m_Renderer->GetSurface();
                        window = mainWindow;
                    }
#else
                    // Non-Windows: windowHandle should be GLFWwindow*
                    // If it's not the main window, something is wrong
                    std::cerr << "Warning: Different GLFW window provided, using main window surface" << std::endl;
                    surface = m_Renderer->GetSurface();
                    window = mainWindow;
#endif
                }
            } else {
                // No window handle provided, use main window surface
                surface = m_Renderer->GetSurface();
                window = mainWindow;
                
                if (surface == VK_NULL_HANDLE) {
                    std::cerr << "Error: No window handle provided and main window surface is null!" << std::endl;
                    return nullptr;
                }
                
                std::cout << "Using main window surface (no window handle provided)" << std::endl;
            }
            
            // Final check: ensure we have a valid surface
            if (surface == VK_NULL_HANDLE) {
                std::cerr << "Failed to get or create surface for swapchain" << std::endl;
                std::cerr << "windowHandle: " << windowHandle << std::endl;
                std::cerr << "mainWindow: " << (mainWindow ? "valid" : "null") << std::endl;
                if (mainWindow) {
                    std::cerr << "mainWindow->GetHandle(): " << mainWindow->GetHandle() << std::endl;
                    std::cerr << "m_Renderer->GetSurface(): " << m_Renderer->GetSurface() << std::endl;
                }
                return nullptr;
            }
            
            std::cout << "Swapchain will use surface: " << surface << " for size: " << desc.width << "x" << desc.height << std::endl;

            // Create VulkanSwapchain with the surface (window can be null for child windows)
            auto swapchain = std::make_unique<VulkanSwapchain>(context, window, surface);
            if (!swapchain->Create()) {
                // If we created a new surface, we should destroy it on failure
                // But for now, we'll let it leak (should be fixed later with proper cleanup)
                return nullptr;
            }

            return swapchain;
        }

        std::unique_ptr<RHI::IShaderModule> VulkanDevice::CreateShaderModule(
            const std::vector<uint32_t>& spirvCode, RHI::ShaderStage stage) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            auto shaderModule = std::make_unique<ShaderModule>(context);
            if (!shaderModule->Create(spirvCode)) {
                return nullptr;
            }

            return std::make_unique<VulkanShaderModule>(context, shaderModule.release(), stage);
        }

        RHI::SemaphoreHandle VulkanDevice::CreateSemaphoreHandle() {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            VkSemaphoreCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkSemaphore semaphore;
            if (vkCreateSemaphore(context->GetDevice(), &createInfo, nullptr, &semaphore) != VK_SUCCESS) {
                return nullptr;
            }

            return reinterpret_cast<RHI::SemaphoreHandle>(semaphore);
        }

        void VulkanDevice::DestroySemaphore(RHI::SemaphoreHandle semaphore) {
            if (!semaphore) return;

            auto* context = m_Renderer->GetDeviceContext();
            if (context) {
                vkDestroySemaphore(context->GetDevice(), static_cast<VkSemaphore>(semaphore), nullptr);
            }
        }

        RHI::FenceHandle VulkanDevice::CreateFence(bool signaled) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            VkFenceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            if (signaled) {
                createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            }

            VkFence fence;
            if (vkCreateFence(context->GetDevice(), &createInfo, nullptr, &fence) != VK_SUCCESS) {
                return nullptr;
            }

            return reinterpret_cast<RHI::FenceHandle>(fence);
        }

        void VulkanDevice::DestroyFence(RHI::FenceHandle fence) {
            if (!fence) return;

            auto* context = m_Renderer->GetDeviceContext();
            if (context) {
                vkDestroyFence(context->GetDevice(), static_cast<VkFence>(fence), nullptr);
            }
        }

        void VulkanDevice::SubmitCommandBuffer(
            RHI::ICommandBuffer* commandBuffer,
            const std::vector<RHI::SemaphoreHandle>& waitSemaphores,
            const std::vector<RHI::SemaphoreHandle>& signalSemaphores,
            RHI::FenceHandle fence) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context || !commandBuffer) {
                return;
            }

            auto* vkCommandBuffer = static_cast<VulkanCommandBuffer*>(commandBuffer);

            std::vector<VkSemaphore> vkWaitSemaphores;
            for (auto handle : waitSemaphores) {
                vkWaitSemaphores.push_back(static_cast<VkSemaphore>(handle));
            }

            std::vector<VkSemaphore> vkSignalSemaphores;
            for (auto handle : signalSemaphores) {
                vkSignalSemaphores.push_back(static_cast<VkSemaphore>(handle));
            }

            std::vector<VkPipelineStageFlags> waitStages(waitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

            VkCommandBuffer vkCmdBuffer = vkCommandBuffer->GetVkCommandBuffer();

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(vkWaitSemaphores.size());
            submitInfo.pWaitSemaphores = vkWaitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &vkCmdBuffer;
            submitInfo.signalSemaphoreCount = static_cast<uint32_t>(vkSignalSemaphores.size());
            submitInfo.pSignalSemaphores = vkSignalSemaphores.data();

            VkResult result = vkQueueSubmit(context->GetGraphicsQueue(), 1, &submitInfo, static_cast<VkFence>(fence));
            if (result != VK_SUCCESS) {
                std::cerr << "Warning: vkQueueSubmit failed with result: " << result << std::endl;
            }
        }

        void VulkanDevice::WaitIdle() {
            if (m_Renderer) {
                m_Renderer->WaitIdle();
            }
        }

        void VulkanDevice::WaitForFence(RHI::FenceHandle fence, uint64_t timeout) {
            if (!fence) return;

            auto* context = m_Renderer->GetDeviceContext();
            if (context) {
                vkWaitForFences(context->GetDevice(), 1, reinterpret_cast<VkFence*>(&fence), VK_TRUE, timeout);
            }
        }

        void VulkanDevice::ResetFence(RHI::FenceHandle fence) {
            if (!fence) return;

            auto* context = m_Renderer->GetDeviceContext();
            if (context) {
                vkResetFences(context->GetDevice(), 1, reinterpret_cast<VkFence*>(&fence));
            }
        }

        RHI::QueueHandle VulkanDevice::GetGraphicsQueue() const {
            if (m_Renderer && m_Renderer->GetDeviceContext()) {
                return reinterpret_cast<RHI::QueueHandle>(m_Renderer->GetDeviceContext()->GetGraphicsQueue());
            }
            return nullptr;
        }

        RHI::QueueHandle VulkanDevice::GetPresentQueue() const {
            if (m_Renderer && m_Renderer->GetDeviceContext()) {
                return reinterpret_cast<RHI::QueueHandle>(m_Renderer->GetDeviceContext()->GetPresentQueue());
            }
            return nullptr;
        }

        const RHI::DeviceInfo& VulkanDevice::GetDeviceInfo() const {
            return m_DeviceInfo;
        }

        // ============================================================================
        // Descriptor set operations implementation
        // ============================================================================

        bool VulkanDevice::IsDescriptorIndexingSupported() const {
            if (m_Renderer) {
                return m_Renderer->IsDescriptorIndexingSupported();
            }
            return false;
        }

        RHI::DescriptorSetLayoutHandle VulkanDevice::CreateDescriptorSetLayout(
            const RHI::DescriptorSetLayoutDescription& desc) {
            // Validate input
            if (desc.bindings.empty()) {
                std::cerr << "Warning: VulkanDevice::CreateDescriptorSetLayout: Empty descriptor set layout description" << std::endl;
                // Return nullptr for empty layouts - caller should handle this
                return nullptr;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                std::cerr << "Error: VulkanDevice::CreateDescriptorSetLayout: Device context is nullptr" << std::endl;
                return nullptr;
            }

            VkDevice device = context->GetDevice();
            if (device == VK_NULL_HANDLE) {
                std::cerr << "Error: VulkanDevice::CreateDescriptorSetLayout: VkDevice is VK_NULL_HANDLE" << std::endl;
                return nullptr;
            }

            // Validate input
            if (!desc.bindings.empty() && desc.bindings[0].count == 0) {
                std::cerr << "Error: VulkanDevice::CreateDescriptorSetLayout: Descriptor set layout description is invalid (binding count is 0)" << std::endl;
                return nullptr;
            }

            std::vector<VkDescriptorSetLayoutBinding> bindings;
            bindings.reserve(desc.bindings.size());

            for (const auto& binding : desc.bindings) {
                // Validate binding - count must be > 0
                if (binding.count == 0) {
                    std::cerr << "Error: VulkanDevice::CreateDescriptorSetLayout: Binding " 
                              << binding.binding << " has count 0, this is invalid. Minimum count is 1." << std::endl;
                    return nullptr; // Fail instead of skipping - this indicates a bug
                }

                VkDescriptorSetLayoutBinding vkBinding{};
                vkBinding.binding = binding.binding;
                // Convert descriptor type from RHI to Vulkan
                vkBinding.descriptorType = Device::ConvertDescriptorType(binding.type);
                vkBinding.descriptorCount = binding.count;
                vkBinding.stageFlags = Device::ConvertShaderStageFlags(binding.stageFlags);
                vkBinding.pImmutableSamplers = nullptr; // No immutable samplers for now
                bindings.push_back(vkBinding);
            }

            if (bindings.empty()) {
                std::cerr << "Warning: VulkanDevice::CreateDescriptorSetLayout: No valid bindings, creating empty layout" << std::endl;
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.flags = 0; // No special flags
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.empty() ? nullptr : bindings.data();
            
            // Disable UPDATE_UNUSED_WHILE_PENDING_BIT for now due to validation errors
            // The flag may not be properly supported or may require additional setup
            // Instead, we'll ensure descriptor sets are updated before command buffer recording
            layoutInfo.pNext = nullptr;
            
            // TODO: Re-enable UPDATE_UNUSED_WHILE_PENDING_BIT once validation errors are resolved
            // This requires:
            // 1. Proper Vulkan version/extension support verification
            // 2. Correct flag values for the Vulkan version being used
            // 3. Proper descriptor pool flags if UPDATE_AFTER_BIND_BIT is used
            /*
            if (IsDescriptorIndexingSupported() && !bindings.empty()) {
                std::vector<VkDescriptorBindingFlags> bindingFlags(bindings.size(), 
                    VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT);
                
                VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{};
                flagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                flagsInfo.bindingCount = static_cast<uint32_t>(bindingFlags.size());
                flagsInfo.pBindingFlags = bindingFlags.data();
                
                layoutInfo.pNext = &flagsInfo;
            }
            */

            VkDescriptorSetLayout layout;
            VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
            if (result != VK_SUCCESS) {
                std::cerr << "Error: VulkanDevice::CreateDescriptorSetLayout: Failed to create descriptor set layout! "
                          << "Result: " << result << ", Binding count: " << bindings.size() << std::endl;
                return nullptr;
            }

            return reinterpret_cast<RHI::DescriptorSetLayoutHandle>(layout);
        }

        void VulkanDevice::DestroyDescriptorSetLayout(RHI::DescriptorSetLayoutHandle layout) {
            if (!layout) {
                return;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return;
            }

            VkDescriptorSetLayout vkLayout = reinterpret_cast<VkDescriptorSetLayout>(layout);
            vkDestroyDescriptorSetLayout(context->GetDevice(), vkLayout, nullptr);
        }

        RHI::DescriptorPoolHandle VulkanDevice::CreateDescriptorPool(
            uint32_t maxSets,
            const std::vector<std::pair<RHI::DescriptorType, uint32_t>>& poolSizes) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            VkDevice device = context->GetDevice();
            std::vector<VkDescriptorPoolSize> vkPoolSizes;

            for (const auto& [type, count] : poolSizes) {
                VkDescriptorPoolSize poolSize{};
                poolSize.type = Device::ConvertDescriptorType(type);
                poolSize.descriptorCount = count;
                vkPoolSizes.push_back(poolSize);
            }

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(vkPoolSizes.size());
            poolInfo.pPoolSizes = vkPoolSizes.data();
            poolInfo.maxSets = maxSets;
            poolInfo.flags = 0; // No flags for now

            VkDescriptorPool pool;
            VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &pool);
            if (result != VK_SUCCESS) {
                std::cerr << "Failed to create descriptor pool!" << std::endl;
                return nullptr;
            }

            return reinterpret_cast<RHI::DescriptorPoolHandle>(pool);
        }

        void VulkanDevice::DestroyDescriptorPool(RHI::DescriptorPoolHandle pool) {
            if (!pool) {
                return;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return;
            }

            VkDescriptorPool vkPool = reinterpret_cast<VkDescriptorPool>(pool);
            vkDestroyDescriptorPool(context->GetDevice(), vkPool, nullptr);
        }

        std::vector<RHI::DescriptorSetHandle> VulkanDevice::AllocateDescriptorSets(
            RHI::DescriptorPoolHandle pool,
            const std::vector<RHI::DescriptorSetLayoutHandle>& layouts) {
            std::vector<RHI::DescriptorSetHandle> result;
            if (!pool || layouts.empty()) {
                return result;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return result;
            }

            VkDevice device = context->GetDevice();
            VkDescriptorPool vkPool = reinterpret_cast<VkDescriptorPool>(pool);

            std::vector<VkDescriptorSetLayout> vkLayouts;
            for (auto layout : layouts) {
                vkLayouts.push_back(reinterpret_cast<VkDescriptorSetLayout>(layout));
            }

            std::vector<VkDescriptorSet> vkSets(layouts.size());
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = vkPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(vkLayouts.size());
            allocInfo.pSetLayouts = vkLayouts.data();

            VkResult vkResult = vkAllocateDescriptorSets(device, &allocInfo, vkSets.data());
            if (vkResult != VK_SUCCESS) {
                std::cerr << "Failed to allocate descriptor sets!" << std::endl;
                return result;
            }

            result.reserve(vkSets.size());
            for (VkDescriptorSet set : vkSets) {
                result.push_back(reinterpret_cast<RHI::DescriptorSetHandle>(set));
            }

            return result;
        }

        void VulkanDevice::FreeDescriptorSets(
            RHI::DescriptorPoolHandle pool,
            const std::vector<RHI::DescriptorSetHandle>& sets) {
            if (!pool || sets.empty()) {
                return;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return;
            }

            VkDevice device = context->GetDevice();
            VkDescriptorPool vkPool = reinterpret_cast<VkDescriptorPool>(pool);

            std::vector<VkDescriptorSet> vkSets;
            for (auto set : sets) {
                vkSets.push_back(reinterpret_cast<VkDescriptorSet>(set));
            }

            vkFreeDescriptorSets(device, vkPool, static_cast<uint32_t>(vkSets.size()), vkSets.data());
        }

        void VulkanDevice::UpdateDescriptorSets(
            const std::vector<RHI::DescriptorWrite>& writes) {
            if (writes.empty()) {
                return;
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return;
            }

            VkDevice device = context->GetDevice();
            std::vector<VkWriteDescriptorSet> vkWrites;
            std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos;
            std::vector<std::vector<VkDescriptorImageInfo>> imageInfos;

            for (const auto& write : writes) {
                VkWriteDescriptorSet vkWrite{};
                vkWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                vkWrite.dstSet = reinterpret_cast<VkDescriptorSet>(write.dstSet);
                vkWrite.dstBinding = write.dstBinding;
                vkWrite.dstArrayElement = write.dstArrayElement;
                vkWrite.descriptorType = Device::ConvertDescriptorType(write.descriptorType);
                vkWrite.descriptorCount = 0;

                // Handle buffer info
                if (!write.bufferInfo.empty()) {
                    bufferInfos.emplace_back();
                    for (const auto& bufInfo : write.bufferInfo) {
                        VkDescriptorBufferInfo vkBufInfo{};
                        if (bufInfo.buffer) {
                            // Get VkBuffer from IBuffer
                            auto* vkBuffer = static_cast<VulkanBuffer*>(bufInfo.buffer);
                            vkBufInfo.buffer = vkBuffer->GetVkBuffer();
                        }
                        vkBufInfo.offset = bufInfo.offset;
                        vkBufInfo.range = bufInfo.range;
                        bufferInfos.back().push_back(vkBufInfo);
                    }
                    vkWrite.descriptorCount = static_cast<uint32_t>(bufferInfos.back().size());
                    vkWrite.pBufferInfo = bufferInfos.back().data();
                }

                // Handle image info
                if (!write.imageInfo.empty()) {
                    imageInfos.emplace_back();
                    for (const auto& imgInfo : write.imageInfo) {
                        VkDescriptorImageInfo vkImgInfo{};
                        
                        // Handle different descriptor types
                        if (vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                            // Separate sampler: only needs sampler, no image or imageView
                            vkImgInfo.imageView = VK_NULL_HANDLE;
                            vkImgInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                            if (imgInfo.sampler) {
                                vkImgInfo.sampler = reinterpret_cast<VkSampler>(imgInfo.sampler);
                            } else {
                                std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Sampler is nullptr for SAMPLER binding " 
                                          << write.dstBinding << std::endl;
                                vkImgInfo.sampler = VK_NULL_HANDLE;
                            }
                        } else {
                            // CombinedImageSampler or SampledImage: needs image and imageView
                            if (imgInfo.image) {
                                // Get VkImageView from IImageView
                                if (imgInfo.imageView) {
                                    auto* vkImageView = static_cast<VulkanImageView*>(imgInfo.imageView);
                                    VkImageView vkImageViewHandle = vkImageView->GetVkImageView();
                                    if (vkImageViewHandle == VK_NULL_HANDLE) {
                                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: ImageView handle is VK_NULL_HANDLE for binding " 
                                                  << write.dstBinding << std::endl;
                                        vkImgInfo.imageView = VK_NULL_HANDLE;
                                    } else {
                                        vkImgInfo.imageView = vkImageViewHandle;
                                    }
                                } else {
                                    // If no imageView provided, try to create one from image
                                    auto* vkImage = static_cast<VulkanImage*>(imgInfo.image);
                                    RHI::IImageView* defaultView = vkImage->CreateImageView();
                                    if (defaultView) {
                                        auto* vkImageView = static_cast<VulkanImageView*>(defaultView);
                                        vkImgInfo.imageView = vkImageView->GetVkImageView();
                                    } else {
                                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Failed to create image view for binding " 
                                                  << write.dstBinding << std::endl;
                                        vkImgInfo.imageView = VK_NULL_HANDLE;
                                    }
                                }
                                vkImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        } else {
                            // For non-SAMPLER types, image should not be nullptr
                            if (vkWrite.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
                                std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Image is nullptr for binding " 
                                          << write.dstBinding << " (type: " << static_cast<int>(vkWrite.descriptorType) << ")" << std::endl;
                            }
                            vkImgInfo.imageView = VK_NULL_HANDLE;
                            vkImgInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        }
                            // Handle sampler: can be VkSampler pointer or nullptr
                            if (imgInfo.sampler) {
                                vkImgInfo.sampler = reinterpret_cast<VkSampler>(imgInfo.sampler);
                            } else {
                                vkImgInfo.sampler = VK_NULL_HANDLE;
                            }
                        }
                        imageInfos.back().push_back(vkImgInfo);
                    }
                    vkWrite.descriptorCount = static_cast<uint32_t>(imageInfos.back().size());
                    vkWrite.pImageInfo = imageInfos.back().data();
                    
                    // Validate that all image views are valid (only for types that require imageView)
                    if (vkWrite.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLER) {
                        bool hasInvalidImageView = false;
                        for (const auto& imgInfo : imageInfos.back()) {
                            if (imgInfo.imageView == VK_NULL_HANDLE) {
                                hasInvalidImageView = true;
                                break;
                            }
                        }
                        if (hasInvalidImageView) {
                            std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Some image views are VK_NULL_HANDLE for binding " 
                                      << write.dstBinding << ". This will cause descriptor set binding errors." << std::endl;
                        }
                    }
                }

                // Validate descriptor count before adding
                if (vkWrite.descriptorCount == 0) {
                    std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Descriptor count is 0 for binding " 
                              << vkWrite.dstBinding << " in set " << vkWrite.dstSet 
                              << " (type: " << static_cast<int>(vkWrite.descriptorType) << ")" << std::endl;
                    std::cerr << "  This usually means the write has no bufferInfo or imageInfo. Skipping this write." << std::endl;
                    continue; // Skip this write - it's invalid
                }
                
                // Ensure descriptorCount matches the layout's expected count (should be 1 for most cases)
                // Note: The layout's descriptorCount is set during CreateDescriptorSetLayout
                // We should match it here, but for now we use the actual count from bufferInfo/imageInfo
                
                // Validate that we have valid data for the descriptor type
                if (vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                    // Combined image sampler requires both sampler and imageView
                    if (!vkWrite.pImageInfo || vkWrite.pImageInfo[0].sampler == VK_NULL_HANDLE) {
                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Invalid sampler for binding " 
                                  << vkWrite.dstBinding << " in set " << vkWrite.dstSet << std::endl;
                        continue; // Skip this write
                    }
                    if (vkWrite.pImageInfo[0].imageView == VK_NULL_HANDLE) {
                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Invalid image view for CombinedImageSampler binding " 
                                  << vkWrite.dstBinding << " in set " << vkWrite.dstSet << std::endl;
                        continue; // Skip this write
                    }
                } else if (vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE) {
                    // Sampled image requires valid imageView (sampler is separate)
                    if (!vkWrite.pImageInfo || vkWrite.pImageInfo[0].imageView == VK_NULL_HANDLE) {
                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Invalid image view for binding " 
                                  << vkWrite.dstBinding << " in set " << vkWrite.dstSet << std::endl;
                        continue; // Skip this write
                    }
                } else if (vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER) {
                    // Separate sampler requires valid sampler (no imageView needed)
                    if (!vkWrite.pImageInfo || vkWrite.pImageInfo[0].sampler == VK_NULL_HANDLE) {
                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Invalid sampler for SAMPLER binding " 
                                  << vkWrite.dstBinding << " in set " << vkWrite.dstSet << std::endl;
                        continue; // Skip this write
                    }
                    // For SAMPLER type, imageView should be VK_NULL_HANDLE (it's not used)
                } else if (vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                           vkWrite.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                    if (!vkWrite.pBufferInfo || vkWrite.pBufferInfo[0].buffer == VK_NULL_HANDLE) {
                        std::cerr << "Error: VulkanDevice::UpdateDescriptorSets: Invalid buffer for binding " 
                                  << vkWrite.dstBinding << " in set " << vkWrite.dstSet << std::endl;
                        continue; // Skip this write
                    }
                }
                
                vkWrites.push_back(vkWrite);
            }

            if (!vkWrites.empty()) {
                vkUpdateDescriptorSets(device, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
            }
        }

        void* VulkanDevice::GetDefaultSampler() {
            // Return cached sampler if already created
            if (m_DefaultSampler != VK_NULL_HANDLE) {
                return reinterpret_cast<void*>(m_DefaultSampler);
            }

            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                std::cerr << "Error: VulkanDevice::GetDefaultSampler: Device context is nullptr" << std::endl;
                return nullptr;
            }

            VkDevice device = context->GetDevice();
            
            // Create default sampler with linear filtering and repeat addressing
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.anisotropyEnable = VK_FALSE; // Can be enabled if needed
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f; // Use all mip levels
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;

            VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_DefaultSampler);
            if (result != VK_SUCCESS) {
                std::cerr << "Error: VulkanDevice::GetDefaultSampler: Failed to create sampler!" << std::endl;
                return nullptr;
            }

            return reinterpret_cast<void*>(m_DefaultSampler);
        }


    } // namespace Device
} // namespace FirstEngine
