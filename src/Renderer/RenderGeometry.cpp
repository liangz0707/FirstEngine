#include "FirstEngine/Renderer/RenderGeometry.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/RHI/IDevice.h"
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

            // Create vertex buffer
            uint64_t vertexBufferSize = m_VertexCount * m_VertexStride;
            RHI::BufferUsageFlags usage = static_cast<RHI::BufferUsageFlags>(
                static_cast<uint32_t>(RHI::BufferUsageFlags::VertexBuffer) |
                static_cast<uint32_t>(RHI::BufferUsageFlags::TransferDst)
            );
            RHI::MemoryPropertyFlags properties = RHI::MemoryPropertyFlags::DeviceLocal;

            m_VertexBuffer = device->CreateBuffer(vertexBufferSize, usage, properties);
            if (!m_VertexBuffer) {
                return false;
            }

            // Upload vertex data (TODO: Use staging buffer for proper upload)
            void* mapped = m_VertexBuffer->Map();
            if (mapped) {
                std::memcpy(mapped, vertexData, vertexBufferSize);
                m_VertexBuffer->Unmap();
            }

            // Create index buffer if needed
            if (m_IndexCount > 0 && indexData != nullptr) {
                uint64_t indexBufferSize = m_IndexCount * sizeof(uint32_t);
                RHI::BufferUsageFlags usage = static_cast<RHI::BufferUsageFlags>(
                    static_cast<uint32_t>(RHI::BufferUsageFlags::IndexBuffer) |
                    static_cast<uint32_t>(RHI::BufferUsageFlags::TransferDst)
                );
                RHI::MemoryPropertyFlags properties = RHI::MemoryPropertyFlags::DeviceLocal;

                m_IndexBuffer = device->CreateBuffer(indexBufferSize, usage, properties);
                if (!m_IndexBuffer) {
                    return false;
                }

                // Upload index data
                void* mappedIndex = m_IndexBuffer->Map();
                if (mappedIndex) {
                    std::memcpy(mappedIndex, indexData, indexBufferSize);
                    m_IndexBuffer->Unmap();
                }
            }

            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
