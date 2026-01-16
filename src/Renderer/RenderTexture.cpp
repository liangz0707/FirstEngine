#include "FirstEngine/Renderer/RenderTexture.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Resources/TextureResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include "FirstEngine/RHI/Types.h"
#include <algorithm>
#include <cstring>
#include <climits>

namespace FirstEngine {
    namespace Renderer {

        RenderTexture::RenderTexture() {
            // Auto-register with resource manager
            RenderResourceManager::GetInstance().RegisterResource(this);
        }

        RenderTexture::~RenderTexture() {
            // Unregister from resource manager
            RenderResourceManager::GetInstance().UnregisterResource(this);
            DoDestroy();
        }

        bool RenderTexture::InitializeFromTexture(Resources::TextureResource* textureResource) {
            if (!textureResource) {
                return false;
            }

            m_TextureResource = textureResource;

            // Copy texture data from TextureResource
            m_Width = textureResource->GetWidth();
            m_Height = textureResource->GetHeight();
            m_Channels = textureResource->GetChannels();

            // Determine format from channels
            if (m_Channels == 1) {
                m_Format = RHI::Format::R8G8B8A8_UNORM; // Use R8_UNORM if available
            } else if (m_Channels == 2) {
                m_Format = RHI::Format::R8G8B8A8_UNORM; // Use R8G8_UNORM if available
            } else if (m_Channels == 3) {
                m_Format = RHI::Format::R8G8B8A8_UNORM; // Pad to RGBA
            } else if (m_Channels == 4) {
                m_Format = RHI::Format::R8G8B8A8_UNORM;
            } else {
                return false;
            }

            // Copy texture data
            const void* data = textureResource->GetData();
            uint32_t dataSize = textureResource->GetDataSize();
            if (!data || dataSize == 0) {
                return false;
            }

            m_TextureData.resize(dataSize);
            std::memcpy(m_TextureData.data(), data, dataSize);

            // If channels == 3, we need to pad to RGBA
            if (m_Channels == 3) {
                // Convert RGB to RGBA
                std::vector<uint8_t> rgbaData;
                rgbaData.reserve(m_Width * m_Height * 4);
                const uint8_t* rgbData = m_TextureData.data();
                for (uint32_t i = 0; i < m_Width * m_Height; ++i) {
                    rgbaData.push_back(rgbData[i * 3 + 0]); // R
                    rgbaData.push_back(rgbData[i * 3 + 1]); // G
                    rgbaData.push_back(rgbData[i * 3 + 2]); // B
                    rgbaData.push_back(255); // A
                }
                m_TextureData = std::move(rgbaData);
                m_Channels = 4;
            }

            return true;
        }

        bool RenderTexture::DoCreate(RHI::IDevice* device) {
            if (!device || !m_TextureResource) {
                return false;
            }

            return CreateImage(device) && UploadTextureData(device);
        }

        bool RenderTexture::DoUpdate(RHI::IDevice* device) {
            // Texture typically doesn't need updates, but can be overridden if needed
            if (!device || !m_TextureResource) {
                return false;
            }

            // Re-upload texture data if texture resource data changed
            return UploadTextureData(device);
        }

        void RenderTexture::DoDestroy() {
            // Destroy GPU resources
            m_Image.reset();
            m_TextureData.clear();
        }

        bool RenderTexture::CreateImage(RHI::IDevice* device) {
            if (!device || m_Width == 0 || m_Height == 0) {
                return false;
            }

            // Create image description
            RHI::ImageDescription imageDesc;
            imageDesc.width = m_Width;
            imageDesc.height = m_Height;
            imageDesc.depth = 1;
            imageDesc.mipLevels = 1;
            imageDesc.arrayLayers = 1;
            imageDesc.format = m_Format;
            imageDesc.usage = RHI::ImageUsageFlags::Sampled | RHI::ImageUsageFlags::TransferDst;
            imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

            // Create GPU image
            m_Image = device->CreateImage(imageDesc);
            return m_Image != nullptr;
        }

        bool RenderTexture::UploadTextureData(RHI::IDevice* device) {
            if (!device || !m_Image || m_TextureData.empty()) {
                return false;
            }

            // Calculate texture data size
            uint64_t textureSize = static_cast<uint64_t>(m_TextureData.size());

            // 1. Create staging buffer (host-visible and host-coherent for CPU writes)
            auto stagingBuffer = device->CreateBuffer(
                textureSize,
                RHI::BufferUsageFlags::TransferSrc,
                RHI::MemoryPropertyFlags::HostVisible | RHI::MemoryPropertyFlags::HostCoherent
            );

            if (!stagingBuffer) {
                return false;
            }

            // 2. Copy texture data to staging buffer
            void* mappedData = stagingBuffer->Map();
            if (!mappedData) {
                return false;
            }
            std::memcpy(mappedData, m_TextureData.data(), textureSize);
            stagingBuffer->Unmap();

            // 3. Create command buffer for transfer operations
            auto commandBuffer = device->CreateCommandBuffer();
            if (!commandBuffer) {
                return false;
            }

            // 4. Begin recording commands
            commandBuffer->Begin();

            // 5. Transition image layout from Undefined to TransferDstOptimal
            // Note: TransitionImageLayout uses Format as a temporary solution for layout
            // Format::Undefined represents VK_IMAGE_LAYOUT_UNDEFINED
            // We need to use a format that represents TRANSFER_DST_OPTIMAL
            // For now, we'll use the image's format and rely on the implementation's heuristic
            commandBuffer->TransitionImageLayout(
                m_Image.get(),
                RHI::Format::Undefined,  // oldLayout: UNDEFINED
                m_Format,                // newLayout: will be interpreted as TRANSFER_DST_OPTIMAL by implementation
                1                        // mipLevels
            );

            // 6. Copy buffer to image
            commandBuffer->CopyBufferToImage(
                stagingBuffer.get(),
                m_Image.get(),
                m_Width,
                m_Height
            );

            // 7. Transition image layout from TransferDstOptimal to ShaderReadOnlyOptimal
            // For shader read, we'll use the image format again
            // The implementation should interpret this as SHADER_READ_ONLY_OPTIMAL
            // Note: This is a workaround - ideally we'd have a proper ImageLayout enum
            commandBuffer->TransitionImageLayout(
                m_Image.get(),
                m_Format,                // oldLayout: TRANSFER_DST_OPTIMAL (heuristic)
                m_Format,                // newLayout: SHADER_READ_ONLY_OPTIMAL (heuristic)
                1                        // mipLevels
            );

            // 8. End recording
            commandBuffer->End();

            // 9. Submit command buffer and wait for completion
            // Create a fence to wait for completion
            auto fence = device->CreateFence(false);
            if (!fence) {
                return false;
            }

            device->SubmitCommandBuffer(
                commandBuffer.get(),
                {},  // waitSemaphores
                {},  // signalSemaphores
                fence // fence
            );

            // 10. Wait for fence (ensures upload is complete)
            device->WaitForFence(fence, UINT64_MAX);
            device->ResetFence(fence);
            device->DestroyFence(fence);

            // Staging buffer will be automatically destroyed when unique_ptr goes out of scope
            // Command buffer will be automatically destroyed when unique_ptr goes out of scope

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
