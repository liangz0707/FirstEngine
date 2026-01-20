#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <string>
#include <vector>
#include <cstdint>

namespace FirstEngine {
    namespace Renderer {

        // Simple MD5 hash utility for shader code identification
        // Returns MD5 hash as hex string (32 characters)
        class FE_RENDERER_API ShaderHash {
        public:
            // Compute MD5 hash of data
            static std::string ComputeMD5(const std::vector<uint8_t>& data);
            
            // Compute MD5 hash of SPIR-V code
            static std::string ComputeMD5(const std::vector<uint32_t>& spirvCode);
            
            // Compute MD5 hash of string
            static std::string ComputeMD5(const std::string& data);

        private:
            // MD5 implementation
            struct MD5Context {
                uint32_t state[4];
                uint32_t count[2];
                uint8_t buffer[64];
            };

            static void MD5Init(MD5Context* ctx);
            static void MD5Update(MD5Context* ctx, const uint8_t* data, size_t len);
            static void MD5Final(uint8_t digest[16], MD5Context* ctx);
            static void MD5Transform(uint32_t state[4], const uint8_t block[64]);
            static void Encode(uint8_t* output, const uint32_t* input, size_t len);
            static void Decode(uint32_t* output, const uint8_t* input, size_t len);
        };

    } // namespace Renderer
} // namespace FirstEngine
