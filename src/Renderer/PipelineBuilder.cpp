#include "FirstEngine/Renderer/PipelineBuilder.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/RHI/Types.h"
#include <stdexcept>
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        PipelineBuilder::PipelineBuilder(RHI::IDevice* device)
            : m_Device(device) {
        }

        PipelineBuilder::~PipelineBuilder() = default;

        RHI::Format PipelineBuilder::ParseFormat(const std::string& formatStr) {
            if (formatStr == "R8G8B8A8_UNORM") return RHI::Format::R8G8B8A8_UNORM;
            if (formatStr == "R8G8B8A8_SRGB") return RHI::Format::R8G8B8A8_SRGB;
            if (formatStr == "B8G8R8A8_UNORM") return RHI::Format::B8G8R8A8_UNORM;
            if (formatStr == "B8G8R8A8_SRGB") return RHI::Format::B8G8R8A8_SRGB;
            if (formatStr == "D32_SFLOAT") return RHI::Format::D32_SFLOAT;
            if (formatStr == "D24_UNORM_S8_UINT") return RHI::Format::D24_UNORM_S8_UINT;
            return RHI::Format::Undefined;
        }

        ResourceType PipelineBuilder::ParseResourceType(const std::string& typeStr) {
            if (typeStr == "texture") return ResourceType::Texture;
            if (typeStr == "attachment") return ResourceType::Attachment;
            if (typeStr == "buffer") return ResourceType::Buffer;
            return ResourceType::Texture;
        }

        bool PipelineBuilder::BuildFromConfig(const PipelineConfig& config, FrameGraph& frameGraph) {
            // Add resources
            for (const auto& resourceConfig : config.resources) {
                ResourceType type = ParseResourceType(resourceConfig.type);
                std::unique_ptr<ResourceDescription> desc;
                
                if (type == ResourceType::Attachment) {
                    RHI::Format format = ParseFormat(resourceConfig.format);
                    bool hasDepth = (resourceConfig.format == "D32_SFLOAT" || resourceConfig.format == "D24_UNORM_S8_UINT");
                    desc = std::make_unique<AttachmentResource>(
                        resourceConfig.name,
                        resourceConfig.width,
                        resourceConfig.height,
                        format,
                        hasDepth
                    );
                } else if (type == ResourceType::Buffer) {
                    desc = std::make_unique<BufferResource>(
                        resourceConfig.name,
                        resourceConfig.size,
                        RHI::BufferUsageFlags::UniformBuffer // Default usage, should be configurable
                    );
                } else {
                    desc = std::make_unique<ResourceDescription>(type, resourceConfig.name);
                }

                frameGraph.AddResource(resourceConfig.name, *desc);
            }

            // Add nodes (Passes)
            for (const auto& passConfig : config.passes) {
                auto* node = frameGraph.AddNode(passConfig.name);
                if (!node) {
                    return false;
                }

                // Set read/write resources
                for (const auto& readRes : passConfig.readResources) {
                    node->AddReadResource(readRes);
                }
                for (const auto& writeRes : passConfig.writeResources) {
                    node->AddWriteResource(writeRes);
                }

                // Set node type (for execution plan)
                node->SetType(passConfig.type);

                // Set execution callback (based on type)
                // Callbacks now return RenderCommandList instead of writing to CommandBuffer
                // sceneCommands parameter contains scene rendering commands to be merged into geometry/forward passes
                if (passConfig.type == "geometry") {
                    node->SetExecuteCallback([passConfig](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                        // Geometry Pass: Render geometry to G-Buffer
                        RenderCommandList cmdList;
                        
                        // TODO: Add BeginRenderPass command for G-Buffer
                        // TODO: Add resource layout transitions if needed
                        
                        // Merge scene rendering commands into this pass
                        // Scene commands contain BindPipeline, BindVertexBuffers, DrawIndexed, etc.
                        if (sceneCommands && !sceneCommands->IsEmpty()) {
                            const auto& sceneCmds = sceneCommands->GetCommands();
                            for (const auto& cmd : sceneCmds) {
                                cmdList.AddCommand(cmd);
                            }
                        }
                        
                        // TODO: Add EndRenderPass command
                        
                        return cmdList;
                    });
                } else if (passConfig.type == "lighting") {
                    node->SetExecuteCallback([passConfig](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                        // Lighting Pass: Perform lighting calculation using G-Buffer
                        // TODO: Implement lighting calculation
                        RenderCommandList cmdList;
                        // Lighting pass doesn't use scene commands
                        return cmdList;
                    });
                } else if (passConfig.type == "forward") {
                    node->SetExecuteCallback([passConfig](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                        // Forward Pass: Forward rendering
                        RenderCommandList cmdList;
                        
                        // TODO: Add BeginRenderPass command
                        // TODO: Add resource layout transitions if needed
                        
                        // Merge scene rendering commands into this pass
                        // Forward rendering directly renders scene geometry
                        if (sceneCommands && !sceneCommands->IsEmpty()) {
                            const auto& sceneCmds = sceneCommands->GetCommands();
                            for (const auto& cmd : sceneCmds) {
                                cmdList.AddCommand(cmd);
                            }
                        }
                        
                        // TODO: Add EndRenderPass command
                        
                        return cmdList;
                    });
                } else if (passConfig.type == "postprocess") {
                    node->SetExecuteCallback([passConfig](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                        // Post-process Pass: Post-processing
                        // TODO: Implement post-processing
                        RenderCommandList cmdList;
                        // Post-process pass doesn't use scene commands
                        return cmdList;
                    });
                }
            }

            // Don't compile here - compilation will happen in OnPrepareFrameGraph
            // after building the execution plan
            return true;
        }

        PipelineConfig PipelineBuilder::CreateDeferredRenderingConfig(uint32_t width, uint32_t height) {
            PipelineConfig config;
            config.name = "DeferredRendering";
            config.width = width;
            config.height = height;

            // G-Buffer resources
            ResourceConfig gBufferAlbedo;
            gBufferAlbedo.name = "GBufferAlbedo";
            gBufferAlbedo.type = "attachment";
            gBufferAlbedo.width = width;
            gBufferAlbedo.height = height;
            gBufferAlbedo.format = "R8G8B8A8_UNORM";
            config.resources.push_back(gBufferAlbedo);

            ResourceConfig gBufferNormal;
            gBufferNormal.name = "GBufferNormal";
            gBufferNormal.type = "attachment";
            gBufferNormal.width = width;
            gBufferNormal.height = height;
            gBufferNormal.format = "R8G8B8A8_UNORM";
            config.resources.push_back(gBufferNormal);

            ResourceConfig gBufferDepth;
            gBufferDepth.name = "GBufferDepth";
            gBufferDepth.type = "attachment";
            gBufferDepth.width = width;
            gBufferDepth.height = height;
            gBufferDepth.format = "D32_SFLOAT";
            config.resources.push_back(gBufferDepth);

            // Final output
            ResourceConfig finalOutput;
            finalOutput.name = "FinalOutput";
            finalOutput.type = "attachment";
            finalOutput.width = width;
            finalOutput.height = height;
            finalOutput.format = "B8G8R8A8_UNORM";
            config.resources.push_back(finalOutput);

            // Geometry Pass
            PassConfig geometryPass;
            geometryPass.name = "GeometryPass";
            geometryPass.type = "geometry";
            geometryPass.writeResources.push_back("GBufferAlbedo");
            geometryPass.writeResources.push_back("GBufferNormal");
            geometryPass.writeResources.push_back("GBufferDepth");
            config.passes.push_back(geometryPass);

            // Lighting Pass
            PassConfig lightingPass;
            lightingPass.name = "LightingPass";
            lightingPass.type = "lighting";
            lightingPass.readResources.push_back("GBufferAlbedo");
            lightingPass.readResources.push_back("GBufferNormal");
            lightingPass.readResources.push_back("GBufferDepth");
            lightingPass.writeResources.push_back("FinalOutput");
            config.passes.push_back(lightingPass);

            return config;
        }

    } // namespace Renderer
} // namespace FirstEngine
