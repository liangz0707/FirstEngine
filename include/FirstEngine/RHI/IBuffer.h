#pragma once

#include "FirstEngine/RHI/Export.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace RHI {

        // Buffer interface
        class FE_RHI_API IBuffer {
        public:
            virtual ~IBuffer() = default;
            virtual uint64_t GetSize() const = 0;
            virtual void* Map() = 0;
            virtual void Unmap() = 0;
            virtual void UpdateData(const void* data, uint64_t size, uint64_t offset = 0) = 0;
        };

    } // namespace RHI
} // namespace FirstEngine
