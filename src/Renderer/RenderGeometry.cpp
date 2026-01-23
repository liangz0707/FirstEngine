#include "FirstEngine/Renderer/RenderGeometry.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ICommandBuffer.h"  // Need complete definition for Begin(), CopyBuffer(), End()
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/Types.h"
#include <cstring>

namespace FirstEngine {
    namespace Renderer {

        RenderGeometry::RenderGeometry() {
            // Auto-register with resource manager
            RenderResourceManager::GetInstance().RegisterResource(this);
        }

        RenderGeometry::~RenderGeometry() {
            // Unregister from resource manager
            RenderResourceManager::GetInstance().UnregisterResource(this);
            DoDestroy();
        }

        bool RenderGeometry::InitializeFromMesh(Resources::MeshHandle mesh) {
            if (!mesh) {
                return false;
            }

            m_MeshResource = mesh;
            m_VertexCount = mesh->GetVertexCount();
            m_IndexCount = mesh->GetIndexCount();
            m_VertexStride = mesh->GetVertexStride();
            m_FirstIndex = 0;
            m_FirstVertex = 0;

            // State is already Uninitialized - GPU resources will be created via ScheduleCreate()
            return true;
        }

        bool RenderGeometry::DoCreate(RHI::IDevice* device) {
            if (!device || !m_MeshResource) {
                return false;
            }

            return CreateBuffers(device);
        }

        bool RenderGeometry::DoUpdate(RHI::IDevice* device) {
            // Geometry typically doesn't need updates, but can be overridden if needed
            // For now, just recreate buffers if mesh data changed
            if (!device || !m_MeshResource) {
                return false;
            }

            // Destroy old buffers
            m_VertexBuffer.reset();
            m_IndexBuffer.reset();

            // Recreate buffers
            return CreateBuffers(device);
        }

        void RenderGeometry::DoDestroy() {
            // Destroy GPU resources
            m_VertexBuffer.reset();
            m_IndexBuffer.reset();
        }

        bool RenderGeometry::CreateBuffers(RHI::IDevice* device) {
            if (!device || !m_MeshResource) {
                return false;
            }

            const void* vertexData = m_MeshResource->GetVertexData();
            const void* indexData = m_MeshResource->GetIndexData();

            if (!vertexData || m_VertexCount == 0) {
                return false;
            }

            // Create vertex buffer (device-local, optimal for GPU access)
            uint64_t vertexBufferSize = m_VertexCount * m_VertexStride;
            RHI::BufferUsageFlags vertexUsage = static_cast<RHI::BufferUsageFlags>(
                static_cast<uint32_t>(RHI::BufferUsageFlags::VertexBuffer) |
                static_cast<uint32_t>(RHI::BufferUsageFlags::TransferDst)
            );
            RHI::MemoryPropertyFlags vertexProperties = RHI::MemoryPropertyFlags::DeviceLocal;

            m_VertexBuffer = device->CreateBuffer(vertexBufferSize, vertexUsage, vertexProperties);
            if (!m_VertexBuffer) {
                return false;
            }

            // Create staging buffer for vertex data upload
            RHI::BufferUsageFlags stagingUsage = RHI::BufferUsageFlags::TransferSrc;
            RHI::MemoryPropertyFlags stagingProperties = static_cast<RHI::MemoryPropertyFlags>(
                static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostVisible) |
                static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostCoherent)
            );

            auto stagingVertexBuffer = device->CreateBuffer(vertexBufferSize, stagingUsage, stagingProperties);
            if (!stagingVertexBuffer) {
                return false;
            }

            // Upload vertex data to staging buffer
            void* mapped = stagingVertexBuffer->Map();
            if (mapped) {
                std::memcpy(mapped, vertexData, vertexBufferSize);
                stagingVertexBuffer->Unmap();
            } else {
                return false;
            }

            // Copy from staging buffer to device-local buffer
            auto cmdBuffer = device->CreateCommandBuffer();
            if (!cmdBuffer) {
                return false;
            }

            cmdBuffer->Begin();
            cmdBuffer->CopyBuffer(stagingVertexBuffer.get(), m_VertexBuffer.get(), vertexBufferSize);
            cmdBuffer->End();

            // Submit and wait for completion
            device->SubmitCommandBuffer(cmdBuffer.get(), {}, {}, nullptr);
            device->WaitIdle();

            // Create index buffer if needed
            if (m_IndexCount > 0 && indexData != nullptr) {
                uint64_t indexBufferSize = m_IndexCount * sizeof(uint32_t);
                RHI::BufferUsageFlags indexUsage = static_cast<RHI::BufferUsageFlags>(
                    static_cast<uint32_t>(RHI::BufferUsageFlags::IndexBuffer) |
                    static_cast<uint32_t>(RHI::BufferUsageFlags::TransferDst)
                );
                RHI::MemoryPropertyFlags indexProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                m_IndexBuffer = device->CreateBuffer(indexBufferSize, indexUsage, indexProperties);
                if (!m_IndexBuffer) {
                    return false;
                }

                // Create staging buffer for index data upload
                auto stagingIndexBuffer = device->CreateBuffer(indexBufferSize, stagingUsage, stagingProperties);
                if (!stagingIndexBuffer) {
                    return false;
                }

                // Upload index data to staging buffer
                void* mappedIndex = stagingIndexBuffer->Map();
                if (mappedIndex) {
                    std::memcpy(mappedIndex, indexData, indexBufferSize);
                    stagingIndexBuffer->Unmap();
                } else {
                    return false;
                }

                // Copy from staging buffer to device-local buffer
                auto indexCmdBuffer = device->CreateCommandBuffer();
                if (!indexCmdBuffer) {
                    return false;
                }

                indexCmdBuffer->Begin();
                indexCmdBuffer->CopyBuffer(stagingIndexBuffer.get(), m_IndexBuffer.get(), indexBufferSize);
                indexCmdBuffer->End();

                // Submit and wait for completion
                device->SubmitCommandBuffer(indexCmdBuffer.get(), {}, {}, nullptr);
                device->WaitIdle();
            }

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
