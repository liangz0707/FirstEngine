#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace RHI {
        class IDevice;
        class IShaderModule;
    }

    namespace Renderer {

        // ShaderModuleTools - utility class for managing GPU shader modules
        // Manages Device and creates ShaderModules from SPIR-V code
        // Uses shaderID + MD5 hash as cache key to avoid duplicate module creation
        class FE_RENDERER_API ShaderModuleTools {
        public:
            // Get singleton instance
            static ShaderModuleTools& GetInstance();
            static void Shutdown();

            // Initialize with device (required for creating shader modules)
            void Initialize(RHI::IDevice* device);

            // Shutdown and cleanup
            void Cleanup();

            // Get or create shader module by shaderID and MD5 hash
            // shaderID: Collection ID
            // md5Hash: MD5 hash of the SPIR-V code for this stage
            // spirvCode: SPIR-V code (used if module doesn't exist in cache)
            // stage: Shader stage
            // Returns the shader module, or nullptr on failure
            RHI::IShaderModule* GetOrCreateShaderModule(
                uint64_t shaderID,
                const std::string& md5Hash,
                const std::vector<uint32_t>& spirvCode,
                ShaderStage stage
            );

            // Get shader module from cache (returns nullptr if not found)
            RHI::IShaderModule* GetShaderModule(uint64_t shaderID, const std::string& md5Hash, ShaderStage stage) const;

            // Check if shader module exists in cache
            bool HasShaderModule(uint64_t shaderID, const std::string& md5Hash, ShaderStage stage) const;

            // Clear cache (useful for device recreation)
            void ClearCache();

            // Check if initialized
            bool IsInitialized() const { return m_Device != nullptr; }

        private:
            ShaderModuleTools() = default;
            ~ShaderModuleTools() = default;
            ShaderModuleTools(const ShaderModuleTools&) = delete;
            ShaderModuleTools& operator=(const ShaderModuleTools&) = delete;

            static ShaderModuleTools* s_Instance;

            RHI::IDevice* m_Device = nullptr;

            // Cache key: shaderID + stage + md5Hash -> ShaderModule
            struct CacheKey {
                uint64_t shaderID;
                uint32_t stage;  // Renderer::ShaderStage as uint32_t
                std::string md5Hash;

                bool operator==(const CacheKey& other) const {
                    return shaderID == other.shaderID && stage == other.stage && md5Hash == other.md5Hash;
                }
            };

            struct CacheKeyHash {
                size_t operator()(const CacheKey& key) const {
                    return std::hash<uint64_t>()(key.shaderID) ^
                           (std::hash<uint32_t>()(key.stage) << 1) ^
                           (std::hash<std::string>()(key.md5Hash) << 2);
                }
            };

            // Shader module cache: (shaderID, stage, md5Hash) -> ShaderModule
            std::unordered_map<CacheKey, std::unique_ptr<RHI::IShaderModule>, CacheKeyHash> m_ShaderCache;

            // Helper: Map Renderer::ShaderStage (as uint32_t) to RHI::ShaderStage
            RHI::ShaderStage MapShaderStage(uint32_t stage) const;
        };

    } // namespace Renderer
} // namespace FirstEngine
