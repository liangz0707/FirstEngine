#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Render pass interface
        class FE_RHI_API IRenderPass {
        public:
            virtual ~IRenderPass() = default;
        };

    } // namespace RHI
} // namespace FirstEngine
