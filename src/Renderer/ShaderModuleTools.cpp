#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        ShaderModuleTools* ShaderModuleTools::s_Instance = nullptr;

        ShaderModuleTools& ShaderModuleTools::GetInstance() {
            if (!s_Instance) {
                s_Instance = new ShaderModuleTools();
            }
            return *s_Instance;
        }

        void ShaderModuleTools::Shutdown() {
            if (s_Instance) {
                s_Instance->Cleanup();
                delete s_Instance;
                s_Instance = nullptr;
            }
        }

        void ShaderModuleTools::Initialize(RHI::IDevice* device) {
            m_Device = device;
        }

        void ShaderModuleTools::Cleanup() {
            ClearCache();
            m_Device = nullptr;
        }

        RHI::IShaderModule* ShaderModuleTools::GetOrCreateShaderModule(
            uint64_t shaderID,
            const std::string& md5Hash,
            const std::vector<uint32_t>& spirvCode,
            ShaderStage stage) {
            
            if (!m_Device || spirvCode.empty() || md5Hash.empty()) {
                return nullptr;
            }

            // Check cache first
            CacheKey key;
            key.shaderID = shaderID;
            key.stage = static_cast<uint32_t>(stage);
            key.md5Hash = md5Hash;

            auto it = m_ShaderCache.find(key);
            if (it != m_ShaderCache.end()) {
                return it->second.get();
            }

            // Create new shader module
            RHI::ShaderStage rhiStage = MapShaderStage(static_cast<uint32_t>(stage));
            auto shaderModule = m_Device->CreateShaderModule(spirvCode, rhiStage);
            
            if (!shaderModule) {
                return nullptr;
            }

            // Store in cache
            RHI::IShaderModule* modulePtr = shaderModule.get();
            m_ShaderCache[key] = std::move(shaderModule);
            
            return modulePtr;
        }

        RHI::IShaderModule* ShaderModuleTools::GetShaderModule(uint64_t shaderID, const std::string& md5Hash, ShaderStage stage) const {
            CacheKey key;
            key.shaderID = shaderID;
            key.stage = static_cast<uint32_t>(stage);
            key.md5Hash = md5Hash;

            auto it = m_ShaderCache.find(key);
            if (it != m_ShaderCache.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        bool ShaderModuleTools::HasShaderModule(uint64_t shaderID, const std::string& md5Hash, ShaderStage stage) const {
            CacheKey key;
            key.shaderID = shaderID;
            key.stage = static_cast<uint32_t>(stage);
            key.md5Hash = md5Hash;
            return m_ShaderCache.find(key) != m_ShaderCache.end();
        }

        void ShaderModuleTools::ClearCache() {
            m_ShaderCache.clear();
        }

        RHI::ShaderStage ShaderModuleTools::MapShaderStage(uint32_t stage) const {
            // Map Renderer::ShaderStage enum values to RHI::ShaderStage
            // Renderer::ShaderStage: Vertex=0, Fragment=1, Geometry=2, TessellationControl=3, TessellationEvaluation=4, Compute=5
            switch (stage) {
                case 0: // ShaderStage::Vertex
                    return RHI::ShaderStage::Vertex;
                case 1: // ShaderStage::Fragment
                    return RHI::ShaderStage::Fragment;
                case 2: // ShaderStage::Geometry
                    return RHI::ShaderStage::Geometry;
                case 3: // ShaderStage::TessellationControl
                    return RHI::ShaderStage::TessellationControl;
                case 4: // ShaderStage::TessellationEvaluation
                    return RHI::ShaderStage::TessellationEvaluation;
                case 5: // ShaderStage::Compute
                    return RHI::ShaderStage::Compute;
                default:
                    return RHI::ShaderStage::Vertex;
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
