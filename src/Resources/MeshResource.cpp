#include "FirstEngine/Resources/MeshResource.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/MeshLoader.h"
#include "FirstEngine/Resources/ModelLoader.h"
#include <cstring>
#include <algorithm>
#include <cctype>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        MeshResource::MeshResource() {
            m_Metadata.refCount = 0;
            m_Metadata.isLoaded = false;
        }

        MeshResource::~MeshResource() = default;

        bool MeshResource::Initialize(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, uint32_t vertexStride) {
            if (vertices.empty()) {
                return false;
            }

            m_VertexCount = static_cast<uint32_t>(vertices.size());
            m_IndexCount = static_cast<uint32_t>(indices.size());
            m_VertexStride = vertexStride > 0 ? vertexStride : sizeof(Vertex);

            // Copy vertex data
            m_VertexData.resize(m_VertexCount * m_VertexStride);
            std::memcpy(m_VertexData.data(), vertices.data(), m_VertexCount * sizeof(Vertex));

            // Copy index data
            if (!indices.empty()) {
                m_IndexData.resize(m_IndexCount * sizeof(uint32_t));
                std::memcpy(m_IndexData.data(), indices.data(), m_IndexCount * sizeof(uint32_t));
            }

            m_Metadata.isLoaded = true;
            m_Metadata.fileSize = m_VertexData.size() + m_IndexData.size();

            return true;
        }

        // IResourceProvider interface implementation
        bool MeshResource::IsFormatSupported(const std::string& filepath) const {
            return ModelLoader::IsFormatSupported(filepath);
        }

        std::vector<std::string> MeshResource::GetSupportedFormats() const {
            return ModelLoader::GetSupportedFormats();
        }

        ResourceLoadResult MeshResource::Load(ResourceID id) {
            if (id == InvalidResourceID) {
                return ResourceLoadResult::UnknownError;
            }

            // Set metadata ID
            m_Metadata.resourceID = id;

            // Call Loader::Load - ResourceManager is used internally by Loader for caching
            // Loader returns both Handle data (vertices, indices) and Metadata
            MeshLoader::LoadResult loadResult = MeshLoader::Load(id);
            
            if (!loadResult.success) {
                return ResourceLoadResult::FileNotFound;
            }

            // Use returned Metadata
            m_Metadata = loadResult.metadata;
            m_Metadata.resourceID = id; // Ensure ID matches

            // Store source mesh file path (for saving)
            m_SourceMeshFile = loadResult.meshFile;

            // Use returned Handle data (vertices, indices) to initialize resource
            if (!loadResult.vertices.empty()) {
                if (!Initialize(loadResult.vertices, loadResult.indices, 
                               loadResult.vertexStride > 0 ? loadResult.vertexStride : sizeof(Vertex))) {
                    return ResourceLoadResult::InvalidFormat;
                }
            }
            
            m_Metadata.isLoaded = true;
            m_Metadata.fileSize = m_VertexData.size() + m_IndexData.size();

            return ResourceLoadResult::Success;
        }

        ResourceLoadResult MeshResource::LoadFromMemory(const void* data, size_t size) {
            // TODO: Implement mesh loading from memory
            (void)data;
            (void)size;
            return ResourceLoadResult::FileNotFound;
        }

        void MeshResource::LoadDependencies() {
            // Meshes typically don't have dependencies
            // This method is here for interface compliance
        }

        bool MeshResource::Save(const std::string& xmlFilePath) const {
            // Save XML with source mesh file path (similar to TextureResource::Save)
            return MeshLoader::Save(xmlFilePath, m_Metadata.name, m_Metadata.resourceID,
                                   m_SourceMeshFile, m_VertexStride);
        }

    } // namespace Resources
} // namespace FirstEngine
