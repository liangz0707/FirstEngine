#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include <vector>
#include <cstdint>

// Forward declarations
namespace FirstEngine {
    namespace Renderer {
        class IRenderResource;
    }
    namespace RHI {
        class IDevice;
        class IImage;
    }
}

namespace FirstEngine {
    namespace Resources {

        // Forward declaration
        class ResourceManager;

        // Texture resource implementation
        // Implements both ITexture (resource itself) and IResourceProvider (loading methods)
        // GPU texture (IRenderResource) is stored in the Handle, not in Component
        class FE_RESOURCES_API TextureResource : public ITexture, public IResourceProvider {
        public:
            TextureResource();
            ~TextureResource() override;

            // IResourceProvider interface
            bool IsFormatSupported(const std::string& filepath) const override;
            std::vector<std::string> GetSupportedFormats() const override;
            ResourceLoadResult Load(ResourceID id) override;
            ResourceLoadResult LoadFromMemory(const void* data, size_t size) override;
            void LoadDependencies() override;

            // Initialize from image data
            bool Initialize(const std::vector<uint8_t>& data, uint32_t width, uint32_t height, uint32_t channels, bool hasAlpha);

            // ITexture interface
            uint32_t GetWidth() const override { return m_Width; }
            uint32_t GetHeight() const override { return m_Height; }
            uint32_t GetChannels() const override { return m_Channels; }
            const void* GetData() const override { return m_Data.data(); }
            uint32_t GetDataSize() const override { return static_cast<uint32_t>(m_Data.size()); }
            bool HasAlpha() const override { return m_HasAlpha; }

            // Save resource to XML file
            bool Save(const std::string& xmlFilePath) const;

            // IResource interface
            const ResourceMetadata& GetMetadata() const override { return m_Metadata; }
            ResourceMetadata& GetMetadata() override { return m_Metadata; }
            void AddRef() override { m_Metadata.refCount++; }
            void Release() override { m_Metadata.refCount--; }
            uint32_t GetRefCount() const override { return m_Metadata.refCount; }

            // Render resource management (internal - creates GPU texture from texture data)
            // Returns true if GPU texture was created or already exists
            // GPU texture creation is handled internally via IRenderResource interface
            bool CreateRenderTexture();
            
            // Check if GPU texture is ready for rendering
            bool IsRenderTextureReady() const;
            
            // Get GPU texture for rendering (does not expose IRenderResource)
            void* GetRenderTexture() const; // Returns RHI::IImage* cast to void*
            
            // Get render data for creating RenderItem (does not expose RenderTexture)
            struct RenderData {
                void* image = nullptr; // RHI::IImage* cast to void*
            };
            bool GetRenderData(RenderData& outData) const;

        private:
            ResourceMetadata m_Metadata;
            std::vector<uint8_t> m_Data;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            uint32_t m_Channels = 0;
            bool m_HasAlpha = false;
            
            // GPU render resource (stored in Handle, not Component)
            // Using void* to avoid including Renderer headers (breaks circular dependency)
            void* m_RenderTexture = nullptr; // RenderTexture* cast to void*
        };

    } // namespace Resources
} // namespace FirstEngine
