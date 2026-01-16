#include "FirstEngine/Renderer/IRenderPipeline.h"

namespace FirstEngine {
    namespace Renderer {

        IRenderPipeline::IRenderPipeline(RHI::IDevice* device)
            : m_Device(device) {
        }

        IRenderPipeline::~IRenderPipeline() = default;

        bool IRenderPipeline::BuildExecutionPlan(FrameGraph& frameGraph, FrameGraphExecutionPlan& plan) {
            // Default implementation: use FrameGraph's BuildExecutionPlan
            return frameGraph.BuildExecutionPlan(plan);
        }

        bool IRenderPipeline::Compile(FrameGraph& frameGraph, const FrameGraphExecutionPlan& plan) {
            // Default implementation: use FrameGraph's Compile
            return frameGraph.Compile(plan);
        }

    } // namespace Renderer
} // namespace FirstEngine
