#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/RHI/IDevice.h"

namespace FirstEngine {
    namespace Renderer {

        DeferredRenderPipeline::DeferredRenderPipeline(RHI::IDevice* device)
            : IRenderPipeline(device)
            , m_GeometryPass(std::make_unique<GeometryPass>())
            , m_LightingPass(std::make_unique<LightingPass>())
            , m_PostProcessPass(std::make_unique<PostProcessPass>()) {
        }

        DeferredRenderPipeline::~DeferredRenderPipeline() = default;

        bool DeferredRenderPipeline::BuildFrameGraph(FrameGraph& frameGraph, const RenderConfig& config) {
            // Clear existing graph
            frameGraph.Clear();

            // Store RenderConfig as member variable (to avoid parameter passing)
            m_RenderConfig = config;

            // Build all passes (all passes are always enabled)
            // Each pass manages its own node creation, resources, resource allocation and configuration
            // Pass receives pipeline pointer to access RenderConfig and Device
            m_GeometryPass->OnBuild(frameGraph, this);
            m_LightingPass->OnBuild(frameGraph, this);
            m_PostProcessPass->OnBuild(frameGraph, this);

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
