#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        // Forward declaration
        class ResourceManager;
        struct Model;

        // Model resource implementation - contains multiple meshes and materials
        // Implements both IModel (resource itself) and IResourceProvider (loading methods)
        class FE_RESOURCES_API ModelResource : public IModel, public IResourceProvider {
        public:
            ModelResource();
            ~ModelResource() override;

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            ModelResource(const ModelResource&) = delete;
            ModelResource& operator=(const ModelResource&) = delete;

            // Allow move constructor and move assignment operator
            ModelResource(ModelResource&&) noexcept = default;
            ModelResource& operator=(ModelResource&&) noexcept = default;

            // IResourceProvider interface
            bool IsFormatSupported(const std::string& filepath) const override;
            std::vector<std::string> GetSupportedFormats() const override;
            ResourceLoadResult Load(ResourceID id) override;
            ResourceLoadResult LoadFromMemory(const void* data, size_t size) override;
            void LoadDependencies() override;

            // IModel interface
            uint32_t GetMeshCount() const override { return static_cast<uint32_t>(m_Meshes.size()); }
            MeshHandle GetMesh(uint32_t index) const override;
            MaterialHandle GetMaterial(uint32_t index) const override;
            const std::string& GetMeshName(uint32_t index) const override;
            
            TextureHandle GetTexture(const std::string& slot) const override;
            void SetTexture(const std::string& slot, TextureHandle texture) override;

            // Set material at index (called during dependency loading)
            void SetMaterial(uint32_t index, MaterialHandle material);

            // Save resource to XML file
            bool Save(const std::string& xmlFilePath) const;

            // IResource interface
            const ResourceMetadata& GetMetadata() const override { return m_Metadata; }
            ResourceMetadata& GetMetadata() override { return m_Metadata; }
            void AddRef() override { m_Metadata.refCount++; }
            void Release() override { m_Metadata.refCount--; }
            uint32_t GetRefCount() const override { return m_Metadata.refCount; }

        private:
            // Parse dependencies from model data (extract material ResourceIDs, no loading)
            // Path handling is hidden - uses ResourceManager singleton to resolve paths
            void ParseDependencies(const Model& model, const std::string& resolvedPath);
            
            // Initialize from loaded model data (called internally by Load after dependencies are loaded)
            // Connects loaded materials to meshes - all path handling is hidden
            bool Initialize(const Model& model);

            ResourceMetadata m_Metadata;
            std::vector<MeshHandle> m_Meshes;
            std::vector<MaterialHandle> m_Materials;
            std::vector<std::string> m_MeshNames;
            
            // Textures directly referenced by model (e.g., lightmap)
            std::unordered_map<std::string, TextureHandle> m_Textures;
            
            // Temporary storage for meshes created during initialization
            std::vector<std::unique_ptr<IMesh>> m_MeshStorage;
        };

    } // namespace Resources
} // namespace FirstEngine
