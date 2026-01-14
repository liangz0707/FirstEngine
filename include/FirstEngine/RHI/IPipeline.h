#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Pipeline interface
        class FE_RHI_API IPipeline {
        public:
            virtual ~IPipeline() = default;
        };

    } // namespace RHI
} // namespace FirstEngine
