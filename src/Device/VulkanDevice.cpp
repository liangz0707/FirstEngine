#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/Device/VulkanRHIWrappers.h"
#include "FirstEngine/Core/Window.h"
#include "FirstEngine/Device/DeviceContext.h"
#include "FirstEngine/Device/MemoryManager.h"
#include "FirstEngine/Device/ShaderModule.h"  // Still needed for VulkanShaderModule wrapper
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
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

        VulkanDevice::VulkanDevice() {
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

            // 创建 VulkanRenderer
            m_Renderer = std::make_unique<VulkanRenderer>(m_Window.get());

            // 填充设备信息
            m_DeviceInfo.deviceName = "Vulkan Device";
            m_DeviceInfo.apiVersion = VK_API_VERSION_1_0;
            m_DeviceInfo.driverVersion = 0;
            m_DeviceInfo.deviceMemory = 0; // 可以从物理设备获取
            m_DeviceInfo.hostMemory = 0;

            return true;
        }

        void VulkanDevice::Shutdown() {
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

            // 限制颜色附件数量（Vulkan 规范要求最多 8 个）
            const uint32_t maxColorAttachments = 8;
            uint32_t colorAttachmentCount = static_cast<uint32_t>(desc.colorAttachments.size());
            if (colorAttachmentCount > maxColorAttachments) {
                std::cerr << "Warning: Color attachment count (" << colorAttachmentCount 
                          << ") exceeds maximum (" << maxColorAttachments 
                          << "). Clamping to maximum." << std::endl;
                colorAttachmentCount = maxColorAttachments;
            }

            // 转换附件描述
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
                
                // initialLayout: 如果未定义，使用 UNDEFINED（允许）
                VkImageLayout initialLayout = ConvertImageLayout(colorAttach.initialLayout);
                if (initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && colorAttach.initialLayout != RHI::Format::Undefined) {
                    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 保持 UNDEFINED
                }
                attachment.initialLayout = initialLayout;
                
                // finalLayout: 不能是 UNDEFINED，使用默认值 COLOR_ATTACHMENT_OPTIMAL
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
                
                // initialLayout: 如果未定义，使用 UNDEFINED（允许）
                VkImageLayout initialLayout = ConvertImageLayout(desc.depthAttachment.initialLayout);
                if (initialLayout == VK_IMAGE_LAYOUT_UNDEFINED && desc.depthAttachment.initialLayout != RHI::Format::Undefined) {
                    initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // 保持 UNDEFINED
                }
                depthAttachment.initialLayout = initialLayout;
                
                // finalLayout: 不能是 UNDEFINED，使用默认值 DEPTH_STENCIL_ATTACHMENT_OPTIMAL
                VkImageLayout finalLayout = ConvertImageLayout(desc.depthAttachment.finalLayout);
                if (finalLayout == VK_IMAGE_LAYOUT_UNDEFINED || desc.depthAttachment.finalLayout == RHI::Format::Undefined) {
                    finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                }
                depthAttachment.finalLayout = finalLayout;
                
                attachments.push_back(depthAttachment);
            }

            // 创建子通道
            std::vector<VkAttachmentReference> colorRefs;
            for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
                VkAttachmentReference ref{};
                ref.attachment = i;
                ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorRefs.push_back(ref);
            }

            VkAttachmentReference depthRef{};
            if (desc.hasDepthAttachment) {
                depthRef.attachment = colorAttachmentCount; // 使用限制后的颜色附件数量
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

            // 子通道依赖关系
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            dependency.dependencyFlags = 0;

            // 创建渲染通道
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

            return std::make_unique<VulkanRenderPass>(context, renderPass);
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

            // 创建着色器阶段
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

            // 顶点输入
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

            // 输入装配
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = ConvertPrimitiveTopology(desc.primitiveTopology);
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            // 视口和裁剪
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

            // 多重采样
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

            // 深度模板
            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = desc.depthStencilState.depthTestEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthWriteEnable = desc.depthStencilState.depthWriteEnable ? VK_TRUE : VK_FALSE;
            depthStencil.depthCompareOp = ConvertCompareOp(desc.depthStencilState.depthCompareOp);
            depthStencil.depthBoundsTestEnable = desc.depthStencilState.depthBoundsTestEnable ? VK_TRUE : VK_FALSE;
            depthStencil.stencilTestEnable = desc.depthStencilState.stencilTestEnable ? VK_TRUE : VK_FALSE;

            // 颜色混合
            std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
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

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
            colorBlending.pAttachments = colorBlendAttachments.data();

            // 动态状态
            VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
            VkPipelineDynamicStateCreateInfo dynamicState{};
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamicStates;

            // 管道布局
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
            // TODO: 实现 ComputePipeline 创建
            return nullptr;
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

            // Get window and surface from renderer
            Core::Window* window = m_Renderer->GetWindow();
            VkSurfaceKHR surface = m_Renderer->GetSurface();
            if (!window || surface == VK_NULL_HANDLE) {
                return nullptr;
            }

            // Create VulkanSwapchain directly (no longer wrapping Device::Swapchain)
            auto swapchain = std::make_unique<VulkanSwapchain>(context, window, surface);
            if (!swapchain->Create()) {
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

        RHI::DescriptorSetLayoutHandle VulkanDevice::CreateDescriptorSetLayout(
            const RHI::DescriptorSetLayoutDescription& desc) {
            auto* context = m_Renderer->GetDeviceContext();
            if (!context) {
                return nullptr;
            }

            VkDevice device = context->GetDevice();
            std::vector<VkDescriptorSetLayoutBinding> bindings;

            for (const auto& binding : desc.bindings) {
                VkDescriptorSetLayoutBinding vkBinding{};
                vkBinding.binding = binding.binding;
                vkBinding.descriptorType = Device::ConvertDescriptorType(binding.type);
                vkBinding.descriptorCount = binding.count;
                vkBinding.stageFlags = Device::ConvertShaderStageFlags(binding.stageFlags);
                vkBinding.pImmutableSamplers = nullptr; // No immutable samplers for now
                bindings.push_back(vkBinding);
            }

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            VkDescriptorSetLayout layout;
            VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &layout);
            if (result != VK_SUCCESS) {
                std::cerr << "Failed to create descriptor set layout!" << std::endl;
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
                        if (imgInfo.image) {
                            // Get VkImageView from IImageView
                            if (imgInfo.imageView) {
                                auto* vkImageView = static_cast<VulkanImageView*>(imgInfo.imageView);
                                vkImgInfo.imageView = vkImageView->GetVkImageView();
                            } else {
                                // If no imageView provided, try to create one from image
                                auto* vkImage = static_cast<VulkanImage*>(imgInfo.image);
                                RHI::IImageView* defaultView = vkImage->CreateImageView();
                                if (defaultView) {
                                    auto* vkImageView = static_cast<VulkanImageView*>(defaultView);
                                    vkImgInfo.imageView = vkImageView->GetVkImageView();
                                } else {
                                    vkImgInfo.imageView = VK_NULL_HANDLE;
                                }
                            }
                            vkImgInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                        } else {
                            vkImgInfo.imageView = VK_NULL_HANDLE;
                            vkImgInfo.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                        }
                        vkImgInfo.sampler = reinterpret_cast<VkSampler>(imgInfo.sampler);
                        imageInfos.back().push_back(vkImgInfo);
                    }
                    vkWrite.descriptorCount = static_cast<uint32_t>(imageInfos.back().size());
                    vkWrite.pImageInfo = imageInfos.back().data();
                }

                vkWrites.push_back(vkWrite);
            }

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(vkWrites.size()), vkWrites.data(), 0, nullptr);
        }


    } // namespace Device
} // namespace FirstEngine
