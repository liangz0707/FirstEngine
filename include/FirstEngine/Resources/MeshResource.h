#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include <vector>
#include <cstdint>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        struct Vertex;
        class ResourceManager;

        // Mesh resource implementation
        // Implements both IMesh (resource itself) and IResourceProvider (loading methods)
        class FE_RESOURCES_API MeshResource : public IMesh, public IResourceProvider {
        public:
            MeshResource();
            ~MeshResource() override;

            // IResourceProvider interface
            bool IsFormatSupported(const std::string& filepath) const override;
            std::vector<std::string> GetSupportedFormats() const override;
            ResourceLoadResult Load(ResourceID id) override;
            ResourceLoadResult LoadFromMemory(const void* data, size_t size) override;
            void LoadDependencies() override;

            // Initialize from vertex and index data
            bool Initialize(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t vertexStride);

            // IMesh interface
            uint32_t GetVertexCount() const override { return m_VertexCount; }
            uint32_t GetIndexCount() const override { return m_IndexCount; }
            const void* GetVertexData() const override { return m_VertexData.data(); }
            const void* GetIndexData() const override { return m_IndexData.data(); }
            uint32_t GetVertexStride() const override { return m_VertexStride; }
            bool IsIndexed() const override { return !m_IndexData.empty(); }

            // Save resource to XML file
            bool Save(const std::string& xmlFilePath) const;

            // IResource interface
            const ResourceMetadata& GetMetadata() const override { return m_Metadata; }
            ResourceMetadata& GetMetadata() override { return m_Metadata; }
            void AddRef() override { m_Metadata.refCount++; }
            void Release() override { m_Metadata.refCount--; }
            uint32_t GetRefCount() const override { return m_Metadata.refCount; }

        private:
            ResourceMetadata m_Metadata;
            std::string m_SourceMeshFile;  // Source mesh file path (fbx, obj, etc.) - similar to TextureResource
            std::vector<uint8_t> m_VertexData;
            std::vector<uint8_t> m_IndexData;
            uint32_t m_VertexCount = 0;
            uint32_t m_IndexCount = 0;
            uint32_t m_VertexStride = 0;
        };

    } // namespace Resources
} // namespace FirstEngine
