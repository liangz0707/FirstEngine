#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/Types.h"
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // PipelineState - describes fixed-function pipeline state
        // Includes blend state, depth/stencil state, rasterization state, etc.
        class FE_RENDERER_API PipelineState {
        public:
            PipelineState() = default;
            ~PipelineState() = default;

            // Rasterization state
            RHI::RasterizationState rasterizationState;

            // Depth stencil state
            RHI::DepthStencilState depthStencilState;

            // Color blend attachments (one per color attachment)
            std::vector<RHI::ColorBlendAttachment> colorBlendAttachments;

            // Primitive topology
            RHI::PrimitiveTopology primitiveTopology = RHI::PrimitiveTopology::TriangleList;

            // Viewport (can be dynamic, but default values)
            struct Viewport {
                float x = 0.0f;
                float y = 0.0f;
                float width = 0.0f;
                float height = 0.0f;
                float minDepth = 0.0f;
                float maxDepth = 1.0f;
            } viewport;

            // Scissor (can be dynamic, but default values)
            struct Scissor {
                int32_t x = 0;
                int32_t y = 0;
                uint32_t width = 0;
                uint32_t height = 0;
            } scissor;

            // Comparison operators for caching
            bool operator==(const PipelineState& other) const;
            bool operator!=(const PipelineState& other) const { return !(*this == other); }

            // Hash for use in unordered containers
            size_t GetHash() const;
        };

    } // namespace Renderer
} // namespace FirstEngine
