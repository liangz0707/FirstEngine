#include "FirstEngine/Renderer/RenderResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/TextureResource.h"
#include <cstring>
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        // RenderMesh implementation
        RenderMesh::RenderMesh() = default;

        RenderMesh::~RenderMesh() {
            // Buffers are managed by RHI device, we just hold references
            m_VertexBuffer = nullptr;
            m_IndexBuffer = nullptr;
        }

        bool RenderMesh::Initialize(RHI::IDevice* device, Resources::MeshHandle meshResource) {
            if (!device || !meshResource) {
                return false;
            }

            m_Device = device;
            m_MeshResource = meshResource;
            m_VertexCount = meshResource->GetVertexCount();
            m_IndexCount = meshResource->GetIndexCount();
            m_VertexStride = meshResource->GetVertexStride();

            // Create vertex buffer
            uint64_t vertexBufferSize = meshResource->GetVertexCount() * m_VertexStride;
            m_VertexBuffer = device->CreateBuffer(
                vertexBufferSize,
                RHI::BufferUsageFlags::VertexBuffer | RHI::BufferUsageFlags::TransferDst,
                RHI::MemoryPropertyFlags::DeviceLocal
            );
            if (!m_VertexBuffer) {
                return false;
            }

            // Upload vertex data
            void* mapped = m_VertexBuffer->Map();
            if (mapped) {
                std::memcpy(mapped, meshResource->GetVertexData(), vertexBufferSize);
                m_VertexBuffer->Unmap();
            }

            // Create index buffer if needed
            if (m_IndexCount > 0) {
                uint64_t indexBufferSize = static_cast<uint64_t>(m_IndexCount) * sizeof(uint32_t);
                m_IndexBuffer = device->CreateBuffer(
                    indexBufferSize,
                    RHI::BufferUsageFlags::IndexBuffer | RHI::BufferUsageFlags::TransferDst,
                    RHI::MemoryPropertyFlags::DeviceLocal
                );
                if (!m_IndexBuffer) {
                    return false;
                }

                // Upload index data
                mapped = m_IndexBuffer->Map();
                if (mapped) {
                    std::memcpy(mapped, meshResource->GetIndexData(), indexBufferSize);
                    m_IndexBuffer->Unmap();
                }
            }

            return true;
        }

        // RenderMaterial implementation
        RenderMaterial::RenderMaterial() = default;

        RenderMaterial::~RenderMaterial() {
            // Textures and buffers are managed by RHI device
            m_Textures.clear();
            m_ParameterBuffer = nullptr;
        }

        bool RenderMaterial::Initialize(RHI::IDevice* device, Resources::MaterialHandle materialResource) {
            if (!device || !materialResource) {
                return false;
            }

            m_Device = device;
            m_MaterialResource = materialResource;
            m_ShaderName = materialResource->GetShaderName();

            // Load textures from material resource
            // Albedo texture
            {
                Resources::TextureHandle albedoTexture = materialResource->GetTexture("Albedo");
                if (albedoTexture) {
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = albedoTexture->GetWidth();
                    imageDesc.height = albedoTexture->GetHeight();
                    imageDesc.format = RHI::Format::R8G8B8A8_UNORM;
                    imageDesc.usage = RHI::ImageUsageFlags::Sampled | RHI::ImageUsageFlags::TransferDst;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto rhiImage = device->CreateImage(imageDesc);
                    if (rhiImage) {
                        // Upload texture data (this would typically be done via staging buffer)
                        // For now, we'll just store the reference
                        m_OwnedImages.push_back(std::move(rhiImage));
                        SetTexture("Albedo", m_OwnedImages.back().get());
                    }
                }
            }

            // Normal texture
            {
                Resources::TextureHandle normalTexture = materialResource->GetTexture("Normal");
                if (normalTexture) {
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = normalTexture->GetWidth();
                    imageDesc.height = normalTexture->GetHeight();
                    imageDesc.format = RHI::Format::R8G8B8A8_UNORM;
                    imageDesc.usage = RHI::ImageUsageFlags::Sampled | RHI::ImageUsageFlags::TransferDst;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto rhiImage = device->CreateImage(imageDesc);
                    if (rhiImage) {
                        m_OwnedImages.push_back(std::move(rhiImage));
                        SetTexture("Normal", m_OwnedImages.back().get());
                    }
                }
            }

            // MetallicRoughness texture
            {
                Resources::TextureHandle mrTexture = materialResource->GetTexture("MetallicRoughness");
                if (mrTexture) {
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = mrTexture->GetWidth();
                    imageDesc.height = mrTexture->GetHeight();
                    imageDesc.format = RHI::Format::R8G8B8A8_UNORM;
                    imageDesc.usage = RHI::ImageUsageFlags::Sampled | RHI::ImageUsageFlags::TransferDst;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto rhiImage = device->CreateImage(imageDesc);
                    if (rhiImage) {
                        m_OwnedImages.push_back(std::move(rhiImage));
                        SetTexture("MetallicRoughness", m_OwnedImages.back().get());
                    }
                }
            }

            // Emissive texture
            {
                Resources::TextureHandle emissiveTexture = materialResource->GetTexture("Emissive");
                if (emissiveTexture) {
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = emissiveTexture->GetWidth();
                    imageDesc.height = emissiveTexture->GetHeight();
                    imageDesc.format = RHI::Format::R8G8B8A8_UNORM;
                    imageDesc.usage = RHI::ImageUsageFlags::Sampled | RHI::ImageUsageFlags::TransferDst;
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto rhiImage = device->CreateImage(imageDesc);
                    if (rhiImage) {
                        m_OwnedImages.push_back(std::move(rhiImage));
                        SetTexture("Emissive", m_OwnedImages.back().get());
                    }
                }
            }

            // Create parameter buffer
            uint32_t paramSize = materialResource->GetParameterDataSize();
            if (paramSize > 0) {
                m_ParameterBuffer = device->CreateBuffer(
                    paramSize,
                    RHI::BufferUsageFlags::UniformBuffer,
                    RHI::MemoryPropertyFlags::HostVisible | RHI::MemoryPropertyFlags::HostCoherent
                );
                if (m_ParameterBuffer != nullptr) {
                    SetParameterData(materialResource->GetParameterData(), paramSize);
                }
            }

            return true;
        }

        void RenderMaterial::SetTexture(const std::string& slot, RHI::IImage* texture) {
            m_Textures[slot] = texture;
        }

        RHI::IImage* RenderMaterial::GetTexture(const std::string& slot) const {
            auto it = m_Textures.find(slot);
            return it != m_Textures.end() ? it->second : nullptr;
        }

        void RenderMaterial::SetParameterData(const void* data, uint32_t size) {
            if (m_ParameterBuffer == nullptr || size == 0) {
                return;
            }

            m_ParameterData.resize(size);
            std::memcpy(m_ParameterData.data(), data, size);

            // Update buffer
            void* mapped = m_ParameterBuffer->Map();
            if (mapped) {
                std::memcpy(mapped, data, size);
                m_ParameterBuffer->Unmap();
            }
        }

    } // namespace Renderer
} // namespace FirstEngine
