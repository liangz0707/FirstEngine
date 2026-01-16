#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/RHI/IImage.h"
#include "FirstEngine/RHI/Types.h"
#include <memory>

namespace FirstEngine {
    namespace Resources {
        class TextureResource;
    }

    namespace Renderer {

        // RenderTexture - GPU texture resource (IRenderResource)
        // Created from TextureResource data and stored in TextureResource handle
        class FE_RENDERER_API RenderTexture : public IRenderResource {
        public:
            RenderTexture();
            ~RenderTexture() override;

            // Initialize from TextureResource (handles all setup internally)
            // textureResource: TextureResource to initialize from
            // Returns true if successful, false otherwise
            bool InitializeFromTexture(Resources::TextureResource* textureResource);

            // IRenderResource interface
            bool DoCreate(RHI::IDevice* device) override;
            bool DoUpdate(RHI::IDevice* device) override;
            void DoDestroy() override;

            // Get GPU texture image
            RHI::IImage* GetImage() const { return m_Image.get(); }

            // Get source texture resource
            Resources::TextureResource* GetTextureResource() const { return m_TextureResource; }

        private:
            // Source texture resource (logical resource, not GPU resource)
            Resources::TextureResource* m_TextureResource = nullptr;
            
            // GPU texture image
            std::unique_ptr<RHI::IImage> m_Image;
            
            // Texture data (copied from TextureResource for GPU upload)
            std::vector<uint8_t> m_TextureData;
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            uint32_t m_Channels = 0;
            RHI::Format m_Format = RHI::Format::R8G8B8A8_UNORM;
            
            // Helper methods
            bool CreateImage(RHI::IDevice* device);
            bool UploadTextureData(RHI::IDevice* device);
        };

    } // namespace Renderer
} // namespace FirstEngine
