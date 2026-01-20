#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/FrameGraph.h"  // FrameGraphBuilder is defined in FrameGraph.h
#include "FirstEngine/Renderer/SceneRenderer.h"

namespace FirstEngine {
    namespace Renderer {

        IRenderPass::IRenderPass(const std::string& name, RenderPassType type)
            : FrameGraphNode(name) {
            // Set type immediately
            SetType(type);
        }

        IRenderPass::~IRenderPass() = default;

        RenderCommandList IRenderPass::OnDraw(FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) {
            // Default implementation: return empty command list
            // Subclasses should override this to provide pass-specific rendering logic
            return RenderCommandList();
        }

        void IRenderPass::SetSceneRenderer(std::unique_ptr<SceneRenderer> sceneRenderer) {
            m_SceneRenderer = std::move(sceneRenderer);
            if (m_SceneRenderer) {
                // Set render flags
                // Camera config will be automatically set in Render() based on pass's camera settings
                m_SceneRenderer->SetRenderFlags(m_RenderFlags);
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
