#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"
#include <vector>

namespace FirstEngine {
    namespace RHI {

        // Shader module interface
        class FE_RHI_API IShaderModule {
        public:
            virtual ~IShaderModule() = default;
            virtual ShaderStage GetStage() const = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
