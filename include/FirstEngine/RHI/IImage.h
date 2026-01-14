#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Image view interface
        class FE_RHI_API IImageView {
        public:
            virtual ~IImageView() = default;
        };

        // Image interface
        class FE_RHI_API IImage {
        public:
            virtual ~IImage() = default;
            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;
            virtual Format GetFormat() const = 0;
            virtual IImageView* CreateImageView() = 0;
            virtual void DestroyImageView(IImageView* imageView) = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
