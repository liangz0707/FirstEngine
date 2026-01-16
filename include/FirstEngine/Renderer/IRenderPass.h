#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include <string>
#include <memory>

// Forward declarations
namespace FirstEngine {
    namespace Renderer {
        class FrameGraph;
        class FrameGraphBuilder;
        class IRenderPipeline;
        class SceneRenderer;
    }
    namespace Resources {
        class Scene;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Abstract base class for render passes
        // IRenderPass inherits from FrameGraphNode, so each Pass IS a Node
        // This allows Pass to directly manage its resources through AddReadResource/AddWriteResource
        // Each render pass is responsible for:
        // 1. Adding itself to the FrameGraph in OnBuild()
        // 2. Adding and allocating resources via AddReadResource/AddWriteResource (with ResourceDescription)
        // 3. Configuring itself (type, execute callback)
        // 4. Optionally creating a SceneRenderer if it needs to render scene objects
        class FE_RENDERER_API IRenderPass : public FrameGraphNode {
        public:
            IRenderPass(const std::string& name, RenderPassType type);
            virtual ~IRenderPass();

            // Build pass: Add self to FrameGraph, add resources, allocate resources and configure in one call
            // This is called during BuildFrameGraph() phase
            // frameGraph: The FrameGraph to add this Pass (as a Node) to
            // pipeline: The render pipeline (for accessing RenderConfig and Device)
            // This method will:
            //   - Call frameGraph.AddNode(this) to add itself as a node (automatically sets execute callback)
            //   - Add resources via AddReadResource/AddWriteResource (with ResourceDescription)
            //   - Resources are automatically allocated by AddReadResource/AddWriteResource
            virtual void OnBuild(FrameGraph& frameGraph, IRenderPipeline* pipeline) = 0;

            // Draw pass: Generate render commands for this pass
            // This is called during Execute() phase to generate render commands
            // The callback receives:
            //   - builder: FrameGraphBuilder for accessing resources
            //   - sceneCommands: Optional scene render commands (from SceneRenderer, if this pass has one)
            // Returns: RenderCommandList containing commands for this pass
            // Default implementation returns empty command list
            // Subclasses should override this to provide pass-specific rendering logic
            virtual RenderCommandList OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands);

            // Optional: Validate pass configuration
            // Returns true if the pass is properly configured
            virtual bool Validate() const { return true; }

            // SceneRenderer management
            // Passes that need to render scene objects should create a SceneRenderer
            // SceneRenderer is owned by the Pass and stores its RenderCommandList internally
            void SetSceneRenderer(std::unique_ptr<SceneRenderer> sceneRenderer);
            SceneRenderer* GetSceneRenderer() { return m_SceneRenderer.get(); }
            const SceneRenderer* GetSceneRenderer() const { return m_SceneRenderer.get(); }
            bool HasSceneRenderer() const { return m_SceneRenderer != nullptr; }

            // Camera configuration for this pass (optional, defaults to global RenderConfig camera)
            // Some passes (e.g., ShadowPass) may use a different camera
            void SetCameraConfig(const CameraConfig& cameraConfig) { m_CameraConfig = cameraConfig; m_UseCustomCamera = true; }
            const CameraConfig& GetCameraConfig() const { return m_CameraConfig; }
            bool UsesCustomCamera() const { return m_UseCustomCamera; }
            void UseGlobalCamera() { m_UseCustomCamera = false; }

            // Render flags for this pass's SceneRenderer (if it has one)
            void SetRenderFlags(RenderObjectFlag flags) { m_RenderFlags = flags; }
            RenderObjectFlag GetRenderFlags() const { return m_RenderFlags; }

        protected:
            // SceneRenderer owned by this pass (nullptr if pass doesn't render scene objects)
            std::unique_ptr<SceneRenderer> m_SceneRenderer;

            // Camera configuration (used if m_UseCustomCamera is true)
            CameraConfig m_CameraConfig;
            bool m_UseCustomCamera = false;

            // Render flags for filtering objects
            RenderObjectFlag m_RenderFlags = RenderObjectFlag::All;
        };

    } // namespace Renderer
} // namespace FirstEngine
