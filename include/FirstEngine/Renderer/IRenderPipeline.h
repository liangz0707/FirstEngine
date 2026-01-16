#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/RHI/IDevice.h"
#include <string>

// Forward declaration
namespace FirstEngine {
    namespace Renderer {
        class RenderConfig;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Abstract interface for render pipelines
        // Each pipeline implementation (Deferred, Forward, etc.) should inherit from this
        class FE_RENDERER_API IRenderPipeline {
        public:
            IRenderPipeline(RHI::IDevice* device);
            virtual ~IRenderPipeline();

            // Get the pipeline name
            virtual std::string GetName() const = 0;

            // Build FrameGraph structure (add nodes and resources)
            // This should be called during initialization
            // Configuration is read from RenderConfig to avoid parameter passing
            virtual bool BuildFrameGraph(FrameGraph& frameGraph, const RenderConfig& config) = 0;

            // Build execution plan from FrameGraph
            // This can be customized by each pipeline implementation
            virtual bool BuildExecutionPlan(FrameGraph& frameGraph, FrameGraphExecutionPlan& plan);

            // Compile FrameGraph (allocate resources)
            // This can be customized by each pipeline implementation
            virtual bool Compile(FrameGraph& frameGraph, const FrameGraphExecutionPlan& plan);

            // Get device
            RHI::IDevice* GetDevice() const { return m_Device; }

        protected:
            RHI::IDevice* m_Device;
        };

    } // namespace Renderer
} // namespace FirstEngine
