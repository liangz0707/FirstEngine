#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceTypeEnum.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include <string>
#include <memory>
#include <cstdint>
#include <vector>

namespace FirstEngine {
    namespace Resources {

        // Forward declarations
        class IMesh;
        class IMaterial;
        class ITexture;
        class IModel;

        // Resource handle types
        using MeshHandle = IMesh*;
        using MaterialHandle = IMaterial*;
        using TextureHandle = ITexture*;
        using ModelHandle = IModel*;
        
        // Unified resource handle (can hold any resource type)
        union ResourceHandle {
            void* ptr;
            MeshHandle mesh;
            MaterialHandle material;
            TextureHandle texture;
            ModelHandle model;
            
            ResourceHandle() : ptr(nullptr) {}
            ResourceHandle(MeshHandle h) : mesh(h) {}
            ResourceHandle(MaterialHandle h) : material(h) {}
            ResourceHandle(TextureHandle h) : texture(h) {}
            ResourceHandle(ModelHandle h) : model(h) {}
        };

        // Resource loading result
        enum class ResourceLoadResult : uint32_t {
            Success = 0,
            FileNotFound = 1,
            InvalidFormat = 2,
            OutOfMemory = 3,
            UnknownError = 4
        };

        // Resource metadata
        // Note: filePath is kept for internal use by ResourceManager only
        // Resource classes should NOT access filePath directly - use resourceID instead
        struct FE_RESOURCES_API ResourceMetadata {
            ResourceID resourceID = 0;  // Primary identifier - the ONLY identifier exposed to resource classes
            std::string filePath;       // Internal: File path (used by ResourceManager for loading, NOT exposed to resource classes)
            std::string name;
            uint64_t fileSize = 0;
            uint64_t loadTime = 0; // in milliseconds
            bool isLoaded = false;
            uint32_t refCount = 0;
            
            // Dependencies (resources this resource depends on) - uses ResourceID only, NO paths
            std::vector<ResourceDependency> dependencies;
        };

        // Base interface for all resources
        class FE_RESOURCES_API IResource {
        public:
            virtual ~IResource() = default;
            virtual const ResourceMetadata& GetMetadata() const = 0;
            virtual ResourceMetadata& GetMetadata() = 0;
            virtual void AddRef() = 0;
            virtual void Release() = 0;
            virtual uint32_t GetRefCount() const = 0;
        };

        // Mesh resource interface
        class FE_RESOURCES_API IMesh : public IResource {
        public:
            virtual ~IMesh() = default;
            virtual uint32_t GetVertexCount() const = 0;
            virtual uint32_t GetIndexCount() const = 0;
            virtual const void* GetVertexData() const = 0;
            virtual const void* GetIndexData() const = 0;
            virtual uint32_t GetVertexStride() const = 0;
            virtual bool IsIndexed() const = 0;
        };

        // Material resource interface
        class FE_RESOURCES_API IMaterial : public IResource {
        public:
            virtual ~IMaterial() = default;
            virtual const std::string& GetShaderName() const = 0;
            virtual void SetShaderName(const std::string& name) = 0;
            virtual void SetTexture(const std::string& slot, TextureHandle texture) = 0;
            virtual TextureHandle GetTexture(const std::string& slot) const = 0;
            virtual const void* GetParameterData() const = 0;
            virtual uint32_t GetParameterDataSize() const = 0;
        };

        // Texture resource interface
        class FE_RESOURCES_API ITexture : public IResource {
        public:
            virtual ~ITexture() = default;
            virtual uint32_t GetWidth() const = 0;
            virtual uint32_t GetHeight() const = 0;
            virtual uint32_t GetChannels() const = 0;
            virtual const void* GetData() const = 0;
            virtual uint32_t GetDataSize() const = 0;
            virtual bool HasAlpha() const = 0;
        };

        // Model resource interface
        class FE_RESOURCES_API IModel : public IResource {
        public:
            virtual ~IModel() = default;
            virtual uint32_t GetMeshCount() const = 0;
            virtual MeshHandle GetMesh(uint32_t index) const = 0;
            virtual MaterialHandle GetMaterial(uint32_t index) const = 0;
            virtual const std::string& GetMeshName(uint32_t index) const = 0;
            
            // Texture access (for lightmap, etc.)
            virtual TextureHandle GetTexture(const std::string& slot) const = 0;
            virtual void SetTexture(const std::string& slot, TextureHandle texture) = 0;
        };

    } // namespace Resources
} // namespace FirstEngine
