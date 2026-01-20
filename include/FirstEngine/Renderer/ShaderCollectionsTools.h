#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/Renderer/ShaderHash.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // ShaderCollectionsTools - utility class for managing shader collections
        // Loads all shaders from Package/Shaders directory, compiles HLSL to SPIR-V,
        // stores SPIR-V code, computes MD5 hashes, and stores ShaderReflection
        // Does NOT manage Device or ShaderModule creation (handled by ShaderModuleTools)
        class FE_RENDERER_API ShaderCollectionsTools {
        public:
            // Get singleton instance
            static ShaderCollectionsTools& GetInstance();
            static void Shutdown();

            // Initialize shader collections tools (loads all shaders from directory)
            // shaderDirectory: path to shaders directory (e.g., "build/Package/Shaders")
            bool Initialize(const std::string& shaderDirectory);

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

            // Get shader reflection by collection ID (for Material access)
            const Shader::ShaderReflection* GetShaderReflection(uint64_t collectionID) const;

            // Get MD5 hash for a specific shader stage in a collection
            std::string GetShaderMD5(uint64_t collectionID, ShaderStage stage) const;

        private:
            ShaderCollectionsTools() = default;
            ~ShaderCollectionsTools() = default;
            ShaderCollectionsTools(const ShaderCollectionsTools&) = delete;
            ShaderCollectionsTools& operator=(const ShaderCollectionsTools&) = delete;

            static ShaderCollectionsTools* s_Instance;

            bool m_Initialized = false;
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
