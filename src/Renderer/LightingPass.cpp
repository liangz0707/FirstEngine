#include "FirstEngine/Renderer/LightingPass.h"
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

        LightingPass::LightingPass()
            : IRenderPass("LightingPass", RenderPassType::Lighting) {
        }

        void LightingPass::OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) {
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

            // Create ElementRenderer for this pass (LightingPass uses Element system)
            RHI::IDevice* device = pipeline->GetDevice();
            if (device && !GetElementRenderer()) {
                auto elementRenderer = std::make_unique<ElementRenderer>(device);
                SetElementRenderer(std::move(elementRenderer));
            }

            // Create fullscreen quad Element for lighting calculation
            if (GetElementRenderer() && !m_QuadGeometry) {
                // Create built-in quad geometry
                m_QuadGeometry = Resources::CreateBuiltinGeometry(Resources::BuiltinGeometryType::Quad);
                if (m_QuadGeometry) {
                    // Initialize the geometry (generates vertex/index data)
                    if (!m_QuadGeometry->Initialize()) {
                        std::cerr << "Warning: LightingPass::OnBuild: Failed to initialize quad geometry" << std::endl;
                        m_QuadGeometry.reset();
                        return;
                    }
                    
                    // Schedule render geometry creation (GPU resources will be created asynchronously)
                    // Note: IsRenderGeometryReady() will return false until ProcessScheduledResources is called
                    if (!m_QuadGeometry->CreateRenderGeometry()) {
                        std::cerr << "Warning: LightingPass::OnBuild: Failed to schedule render geometry creation for quad" << std::endl;
                        m_QuadGeometry.reset();
                        return;
                    }
                    
                    // Create Element and add to ElementRenderer
                    // Geometry will be checked for readiness in OnDraw
                    if (GetElementRenderer()->GetElementCount() == 0) {
                        auto element = std::make_unique<Element>();
                        element->SetShaderName("Light"); // Use Light shader for lighting
                        element->SetGeometry(m_QuadGeometry.get());
                        // Transform is identity (fullscreen quad in clip space)
                        element->SetTransform(glm::mat4(1.0f));
                        
                        // Add Element to ElementRenderer
                        GetElementRenderer()->AddElement(std::move(element));
                    }
                }
            }

            // Declare resource access (automatically adds and allocates resources if needed)
            // Read G-Buffer resources (no description needed, they should already exist)
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_ALBEDO));
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_MATERIAL));
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_NORMAL));
            AddReadResource(FrameGraphResourceNameToString(GBUFFER_DEPTH));

            // Write to final output (with description, will auto-add and allocate if not exists)
            AttachmentResource outputRes(
                FrameGraphResourceNameToString(FINAL_OUTPUT),
                resolution.width,
                resolution.height,
                RHI::Format::B8G8R8A8_UNORM,
                false
            );
            AddWriteResource(FrameGraphResourceNameToString(FINAL_OUTPUT), &outputRes);
        }

        RenderCommandList LightingPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands)  {
            RenderCommandList cmdList;

            // Get render pass and framebuffer from builder
            RHI::IRenderPass* renderPass = builder.GetRenderPass();
            RHI::IFramebuffer* framebuffer = builder.GetFramebuffer();
            
            // Debug: Log render pass and framebuffer status
            if (!renderPass) {
                std::cerr << "Error: LightingPass::OnDraw: renderPass is nullptr from builder" << std::endl;
            }
            if (!framebuffer) {
                std::cerr << "Error: LightingPass::OnDraw: framebuffer is nullptr from builder" << std::endl;
            }
            if (framebuffer && (framebuffer->GetWidth() == 0 || framebuffer->GetHeight() == 0)) {
                std::cerr << "Error: LightingPass::OnDraw: framebuffer has invalid dimensions: " 
                          << framebuffer->GetWidth() << "x" << framebuffer->GetHeight() << std::endl;
            }

            // Lighting Pass: Perform lighting calculation using G-Buffer
            // This pass performs deferred lighting by:
            // 1. Reading G-Buffer textures (Albedo, Normal, Depth)
            // 2. Rendering a fullscreen quad with a lighting shader using Element system
            // 3. Outputting the lit result to FinalOutput
            
            // Get G-Buffer resources from builder
            RHI::IImage* gBufferAlbedo = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_ALBEDO));
            RHI::IImage* gBufferNormal = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_NORMAL));
            RHI::IImage* gBufferMaterial = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_MATERIAL));
            RHI::IImage* gBufferDepth = builder.ReadTexture(FrameGraphResourceNameToString(GBUFFER_DEPTH));
            
            if (!gBufferAlbedo || !gBufferNormal || !gBufferDepth || !gBufferMaterial) {
                std::cerr << "Warning: LightingPass::OnDraw: G-Buffer resources not available" << std::endl;
                return cmdList; // Return empty command list if resources are not available
            }

            // Get ElementRenderer and Element
            auto* elementRenderer = GetElementRenderer();
            if (!elementRenderer || !elementRenderer->HasElements()) {
                std::cerr << "Warning: LightingPass::OnDraw: ElementRenderer or Element not initialized" << std::endl;
                return cmdList;
            }

            // Get Element from ElementRenderer (we only have one Element at index 0)
            Element* element = elementRenderer->GetElement(0);
            if (!element) {
                std::cerr << "Warning: LightingPass::OnDraw: Element not found in ElementRenderer" << std::endl;
                return cmdList;
            }
            
            // Ensure geometry is ready for rendering
            auto* geometry = element->GetGeometry();
            if (geometry && !geometry->IsRenderGeometryReady()) {
                // Try to create render geometry if not ready
                if (!geometry->CreateRenderGeometry()) {
                    std::cerr << "Warning: LightingPass::OnDraw: Failed to create render geometry" << std::endl;
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
                    std::cerr << "Warning: LightingPass::OnDraw: Geometry still not ready after creation attempt" << std::endl;
                    return cmdList;
                }
            }

            // Get ShadingMaterial from Element and set G-Buffer textures
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

            // Only proceed if we have valid render pass and framebuffer
            if (!renderPass || !framebuffer) {
                std::cerr << "Error: LightingPass::OnDraw: renderPass or framebuffer is nullptr" << std::endl;
                std::cerr << "  renderPass: " << (renderPass ? "valid" : "nullptr") << std::endl;
                std::cerr << "  framebuffer: " << (framebuffer ? "valid" : "nullptr") << std::endl;
                return cmdList; // Return empty command list if render pass or framebuffer is invalid
            }
            
            // IMPORTANT: Set textures and update descriptor sets BEFORE adding commands
            // This ensures descriptor sets are updated with the correct layout (SHADER_READ_ONLY_OPTIMAL)
            // The descriptor set layout should match what the image will be when used (after layout transition)
            if (shadingMaterial && shadingMaterial->IsCreated()) {
                // Set G-Buffer textures to ShadingMaterial
                // Note: Binding indices must match Light shader definitions exactly
                // Light shader uses: gAlbedo (binding 1), gNormal (binding 2), gMaterial (binding 3), gDepth (binding 4)
                // These bindings are in Set 0, and LightParams is at binding 0, so textures start at binding 1
                shadingMaterial->SetTexture(0, 1, gBufferAlbedo);     // gAlbedo (binding 1, set 0)
                shadingMaterial->SetTexture(0, 2, gBufferNormal);      // gNormal (binding 2, set 0)
                shadingMaterial->SetTexture(0, 3, gBufferMaterial);     // gMaterial (binding 3, set 0)
                shadingMaterial->SetTexture(0, 4, gBufferDepth);       // gDepth (binding 4, set 0)
                
                // Update descriptor sets with SHADER_READ_ONLY_OPTIMAL layout
                // This is correct because the layout transition commands will execute before the descriptor sets are used
                // Get device from FrameGraph (FrameGraphBuilder doesn't have GetDevice)
                FrameGraph* frameGraph = const_cast<FrameGraph*>(builder.GetGraph());
                if (frameGraph) {
                    RHI::IDevice* device = frameGraph->GetDevice();
                    if (device) {
                        shadingMaterial->FlushParametersToGPU(device);
                    }
                }
            }
            
            // Layout transitions are now automatically handled by FrameGraph based on AddReadResource/AddWriteResource
            // FrameGraph will automatically insert transitions:
            // - Read resources: COLOR_ATTACHMENT_OPTIMAL or DEPTH_STENCIL_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
            // - Write resources: SHADER_READ_ONLY_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL or DEPTH_STENCIL_ATTACHMENT_OPTIMAL

            // Add BeginRenderPass command
            RenderCommand beginCmd;
            beginCmd.type = RenderCommandType::BeginRenderPass;
            beginCmd.params.beginRenderPass.renderPass = renderPass;
            beginCmd.params.beginRenderPass.framebuffer = framebuffer;
            beginCmd.params.beginRenderPass.width = framebuffer->GetWidth();
            beginCmd.params.beginRenderPass.height = framebuffer->GetHeight();
            // Clear color for final output (black background)
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
