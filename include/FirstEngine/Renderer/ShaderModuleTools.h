#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/RHI/IDevice.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace RHI {
        class IDevice;
    }

    namespace Renderer {

        // ShaderModuleTools - utility class for managing shader collections
        // Loads all shaders from Package/Shaders directory, compiles HLSL to SPIR-V,
        // and stores them in ShaderCollections
        class FE_RENDERER_API ShaderModuleTools {
        public:
            // Get singleton instance
            static ShaderModuleTools& GetInstance();
            static void Shutdown();

            // Initialize shader tools (loads all shaders from directory)
            // shaderDirectory: path to shaders directory (e.g., "build/Package/Shaders")
            // device: RHI device for creating shader modules (can be nullptr for lazy creation)
            bool Initialize(const std::string& shaderDirectory, RHI::IDevice* device = nullptr);

            // Shutdown and cleanup
            void Cleanup();

            // Get shader collection by ID
            ShaderCollection* GetCollection(uint64_t id) const;

            // Get shader collection by name
            ShaderCollection* GetCollectionByName(const std::string& name) const;

            // Add a shader collection (returns the ID assigned)
            uint64_t AddCollection(std::unique_ptr<ShaderCollection> collection);

            // Create shader collection from shader files
            // shaderName: base name of shader (e.g., "PBR" for PBR.vert.hlsl and PBR.frag.hlsl)
            // Returns the ID of the created collection, or 0 on failure
            uint64_t CreateCollectionFromFiles(const std::string& shaderName, const std::string& shaderDirectory);

            // Load all shaders from directory
            // Automatically detects shader pairs (e.g., PBR.vert.hlsl + PBR.frag.hlsl)
            bool LoadAllShadersFromDirectory(const std::string& shaderDirectory);

            // Get all collection IDs
            std::vector<uint64_t> GetAllCollectionIDs() const;

            // Check if initialized
            bool IsInitialized() const { return m_Initialized; }

            // Set device (for lazy shader module creation)
            void SetDevice(RHI::IDevice* device);

        private:
            ShaderModuleTools() = default;
            ~ShaderModuleTools() = default;
            ShaderModuleTools(const ShaderModuleTools&) = delete;
            ShaderModuleTools& operator=(const ShaderModuleTools&) = delete;

            static ShaderModuleTools* s_Instance;

            bool m_Initialized = false;
            RHI::IDevice* m_Device = nullptr;
            std::string m_ShaderDirectory;

            // Shader collections by ID
            std::unordered_map<uint64_t, std::unique_ptr<ShaderCollection>> m_Collections;

            // Shader collections by name (for quick lookup)
            std::unordered_map<std::string, uint64_t> m_NameToID;

            // Next available ID
            uint64_t m_NextID = 1;

            // Helper: Detect shader stage from filename
            ShaderStage DetectShaderStage(const std::string& filename) const;

            // Helper: Compile HLSL to SPIR-V
            std::vector<uint32_t> CompileHLSLToSPIRV(const std::string& filepath, ShaderStage stage) const;

            // Helper: Load shader file content
            std::string LoadShaderFile(const std::string& filepath) const;
        };

    } // namespace Renderer
} // namespace FirstEngine
