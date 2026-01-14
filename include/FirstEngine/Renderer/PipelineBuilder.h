#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/PipelineConfig.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"  // Need complete definition of Format enum
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        // Build FrameGraph from configuration file
        class FE_RENDERER_API PipelineBuilder {
        public:
            PipelineBuilder(RHI::IDevice* device);
            ~PipelineBuilder();

            // Build FrameGraph from configuration file
            bool BuildFromConfig(const PipelineConfig& config, FrameGraph& frameGraph);

            // Create deferred rendering pipeline configuration
            static PipelineConfig CreateDeferredRenderingConfig(uint32_t width, uint32_t height);

        private:
            RHI::Format ParseFormat(const std::string& formatStr);
            ResourceType ParseResourceType(const std::string& typeStr);

            RHI::IDevice* m_Device;
        };

    } // namespace Renderer
} // namespace FirstEngine
