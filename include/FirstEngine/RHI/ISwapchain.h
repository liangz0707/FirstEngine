#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Swapchain interface
        class FE_RHI_API ISwapchain {
        public:
            virtual ~ISwapchain() = default;
            virtual bool AcquireNextImage(SemaphoreHandle semaphore, FenceHandle fence, uint32_t& imageIndex) = 0;
            virtual bool Present(uint32_t imageIndex, const std::vector<SemaphoreHandle>& waitSemaphores) = 0;
            virtual uint32_t GetImageCount() const = 0;
            virtual Format GetImageFormat() const = 0;
            virtual void GetExtent(uint32_t& width, uint32_t& height) const = 0;
            virtual IImage* GetImage(uint32_t index) = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
