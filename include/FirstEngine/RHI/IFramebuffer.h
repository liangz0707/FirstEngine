#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Forward declaration
        class IImageView;

        // Framebuffer interface
        class FE_RHI_API IFramebuffer {
        public:
            virtual ~IFramebuffer() = default;
            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
