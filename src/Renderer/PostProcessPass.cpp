#include "FirstEngine/Renderer/PostProcessPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/ElementRenderer.h"
#include "FirstEngine/Renderer/Element.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Resources/BuiltinGeometry.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include <iostream>
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        PostProcessPass::PostProcessPass()
            : IRenderPass("PostProcessPass", RenderPassType::PostProcess) {
        }

        void PostProcessPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
            if (!pipeline) {
                return;
            }

            // Clear resources from previous frame (important: nodes are reused across frames)
            ClearResources();

            // Get RenderConfig from pipeline
            auto* deferredPipeline = dynamic_cast<DeferredRenderPipeline*>(pipeline);
            if (!deferredPipeline) {
                return;
            }
            const auto& resolution = deferredPipeline->GetRenderConfig().GetResolution();

            // Add self to FrameGraph (this Pass IS the Node)
            if (!frameGraph.AddNode(this)) {
                return; // Failed to add node
            }

            // Create ElementRenderer for this pass (PostProcessPass uses Element system)
            RHI::IDevice* device = pipeline->GetDevice();
            if (device && !GetElementRenderer()) {
                auto elementRenderer = std::make_unique<ElementRenderer>(device);
                SetElementRenderer(std::move(elementRenderer));
            }

            // Create fullscreen quad Element for post-processing
            if (GetElementRenderer() && !m_QuadGeometry) {
                // Create built-in quad geometry
                m_QuadGeometry = Resources::CreateBuiltinGeometry(Resources::BuiltinGeometryType::Quad);
                if (m_QuadGeometry) {
                    // Initialize the geometry (generates vertex/index data)
                    if (!m_QuadGeometry->Initialize()) {
                        std::cerr << "Warning: PostProcessPass::OnBuild: Failed to initialize quad geometry" << std::endl;
                        m_QuadGeometry.reset();
                        return;
                    }
                    
                    // Schedule render geometry creation (GPU resources will be created asynchronously)
                    // Note: IsRenderGeometryReady() will return false until ProcessScheduledResources is called
                    if (!m_QuadGeometry->CreateRenderGeometry()) {
                        std::cerr << "Warning: PostProcessPass::OnBuild: Failed to schedule render geometry creation for quad" << std::endl;
                        m_QuadGeometry.reset();
                        return;
                    }
                    
                    // Create Element and add to ElementRenderer
                    // Geometry will be checked for readiness in OnDraw
                    if (GetElementRenderer()->GetElementCount() == 0) {
                        auto element = std::make_unique<Element>();
                        element->SetShaderName("PostProcess"); // Use PostProcess shader
                        element->SetGeometry(m_QuadGeometry.get());
                        // Transform is identity (fullscreen quad in clip space)
                        element->SetTransform(glm::mat4(1.0f));
                        
                        // Add Element to ElementRenderer
                        GetElementRenderer()->AddElement(std::move(element));
                    }
                }
            }

            // Declare resource access (automatically adds and allocates resources if needed)
            // Ping-Pong Buffer pattern: Read from FINAL_OUTPUT, write to PostProcessBuffer
            // This avoids the Vulkan limitation of not being able to use the same image
            // as both attachment (write) and shader resource (read) in the same render pass
            
            // Read final output from LightingPass (no description needed, it should already exist)
            AddReadResource(FrameGraphResourceNameToString(FINAL_OUTPUT));
            
            // Write to PostProcessBuffer (with description, will auto-add and allocate if not exists)
            AttachmentResource outputRes(
                FrameGraphResourceNameToString(FrameGraphResourceName::PostProcessBuffer),
                resolution.width,
                resolution.height,
                RHI::Format::B8G8R8A8_UNORM,
                false
            );
            AddWriteResource(FrameGraphResourceNameToString(FrameGraphResourceName::PostProcessBuffer), &outputRes);
        }

        RenderCommandList PostProcessPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            RenderCommandList cmdList;

            // Get render pass and framebuffer from builder
            RHI::IRenderPass* renderPass = builder.GetRenderPass();
            RHI::IFramebuffer* framebuffer = builder.GetFramebuffer();

            // Post-process Pass: Post-processing
            // This pass applies post-processing effects using Ping-Pong Buffer pattern:
            // 1. Reading the FinalOutput texture (lit result from LightingPass) as input
            // 2. Rendering a fullscreen quad with post-processing shader using Element system
            // 3. Outputting the processed result to PostProcessBuffer (separate from input)
            
            // Get input texture from builder (FINAL_OUTPUT from LightingPass)
            RHI::IImage* inputTexture = builder.ReadTexture(FrameGraphResourceNameToString(FINAL_OUTPUT));
            
            if (!inputTexture) {
                std::cerr << "Warning: PostProcessPass::OnDraw: FinalOutput input texture not available" << std::endl;
                return cmdList; // Return empty command list if resources are not available
            }

            // Get ElementRenderer and Element
            auto* elementRenderer = GetElementRenderer();
            if (!elementRenderer || !elementRenderer->HasElements()) {
                std::cerr << "Warning: PostProcessPass::OnDraw: ElementRenderer or Element not initialized" << std::endl;
                return cmdList;
            }

            // Get Element from ElementRenderer (we only have one Element at index 0)
            Element* element = elementRenderer->GetElement(0);
            if (!element) {
                std::cerr << "Warning: PostProcessPass::OnDraw: Element not found in ElementRenderer" << std::endl;
                return cmdList;
            }
            
            // Ensure geometry is ready for rendering
            auto* geometry = element->GetGeometry();
            if (geometry && !geometry->IsRenderGeometryReady()) {
                // Try to create render geometry if not ready
                if (!geometry->CreateRenderGeometry()) {
                    std::cerr << "Warning: PostProcessPass::OnDraw: Failed to create render geometry" << std::endl;
                    return cmdList;
                }
                
                // Process scheduled resources to ensure geometry is created immediately
                FrameGraph* frameGraph = builder.GetGraph();
                if (frameGraph) {
                    RHI::IDevice* device = frameGraph->GetDevice();
                    if (device) {
                        // Process resources to create the geometry
                        RenderResourceManager::GetInstance().ProcessScheduledResources(device, 0);
                    }
                }
                
                // Check again if geometry is ready
                if (!geometry->IsRenderGeometryReady()) {
                    std::cerr << "Warning: PostProcessPass::OnDraw: Geometry still not ready after creation attempt" << std::endl;
                    return cmdList;
                }
            }

            // Get ShadingMaterial from Element and set input texture
            auto* shadingMaterial = element->GetShadingMaterial();
            if (!shadingMaterial) {
                // Try to create ShadingMaterial if not created yet
                FrameGraph* frameGraph = builder.GetGraph();
                if (frameGraph) {
                    RHI::IDevice* device = frameGraph->GetDevice();
                    if (device && element->CreateShadingMaterial(device)) {
                        shadingMaterial = element->GetShadingMaterial();
                    }
                }
            }

            if (shadingMaterial && shadingMaterial->IsCreated()) {
                // Set input texture to ShadingMaterial
                // PostProcess shader uses: inputTexture (binding 1, set 0)
                // This is the FINAL_OUTPUT from LightingPass, which will be sampled in the shader
                shadingMaterial->SetTexture(0, 1, inputTexture);  // inputTexture
                
                // Force update descriptor sets with new texture
                FrameGraph* frameGraph = builder.GetGraph();
                if (frameGraph) {
                    RHI::IDevice* device = frameGraph->GetDevice();
                    if (device) {
                        shadingMaterial->FlushParametersToGPU(device);
                    }
                }
            }

            // Only proceed if we have valid render pass and framebuffer
            if (!renderPass || !framebuffer) {
                std::cerr << "Error: PostProcessPass::OnDraw: renderPass or framebuffer is nullptr" << std::endl;
                std::cerr << "  renderPass: " << (renderPass ? "valid" : "nullptr") << std::endl;
                std::cerr << "  framebuffer: " << (framebuffer ? "valid" : "nullptr") << std::endl;
                return cmdList; // Return empty command list if render pass or framebuffer is invalid
            }

            // Add BeginRenderPass command
            RenderCommand beginCmd;
            beginCmd.type = RenderCommandType::BeginRenderPass;
            beginCmd.params.beginRenderPass.renderPass = renderPass;
            beginCmd.params.beginRenderPass.framebuffer = framebuffer;
            beginCmd.params.beginRenderPass.width = framebuffer->GetWidth();
            beginCmd.params.beginRenderPass.height = framebuffer->GetHeight();
            // Clear color for post-process output (no clear, load existing)
            beginCmd.params.beginRenderPass.clearColors = {0.0f, 0.0f, 0.0f, 1.0f};
            beginCmd.params.beginRenderPass.clearDepth = 1.0f;
            beginCmd.params.beginRenderPass.clearStencil = 0;
            cmdList.AddCommand(beginCmd);

            // IMPORTANT: elementRenderer->Render() was already called in FrameGraph::Execute
            // Get the commands that were already generated by elementRenderer->Render()
            // These commands include BindPipeline, BindDescriptorSets, DrawIndexed, etc.
            if (elementRenderer->HasRenderCommands()) {
                const RenderCommandList& elementCommands = elementRenderer->GetRenderCommands();
                const auto& elementCmds = elementCommands.GetCommands();
                for (const auto& cmd : elementCmds) {
                    cmdList.AddCommand(cmd);
                }
            }

            // Add EndRenderPass command
            RenderCommand endCmd;
            endCmd.type = RenderCommandType::EndRenderPass;
            cmdList.AddCommand(endCmd);

            return cmdList;
        }

    } // namespace Renderer
} // namespace FirstEngine
