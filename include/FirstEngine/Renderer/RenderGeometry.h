#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/RHI/IBuffer.h"
#include <memory>

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class MeshResource;
        // MeshHandle is defined in ResourceTypes.h as IMesh*
        // MeshResource* is compatible with IMesh*
    }
}

#include "FirstEngine/Resources/ResourceTypes.h"  // For MeshHandle definition

namespace FirstEngine {
    namespace Renderer {

        // RenderGeometry - manages GPU geometry resources (vertex/index buffers)
        // Created from MeshResource, implements IRenderResource for lifecycle management
        class FE_RENDERER_API RenderGeometry : public IRenderResource {
        public:
            RenderGeometry();
            ~RenderGeometry() override;

            // Initialize from mesh resource (sets up data, but doesn't create GPU resources yet)
            // GPU resources will be created via ScheduleCreate() -> DoCreate()
            bool InitializeFromMesh(Resources::MeshHandle mesh);

            // IRenderResource interface
            bool DoCreate(RHI::IDevice* device) override;
            bool DoUpdate(RHI::IDevice* device) override;
            void DoDestroy() override;

            // Get GPU resources (only valid after Initialize)
            RHI::IBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
            RHI::IBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }

            // Get geometry data
            uint32_t GetVertexCount() const { return m_VertexCount; }
            uint32_t GetIndexCount() const { return m_IndexCount; }
            uint32_t GetVertexStride() const { return m_VertexStride; }
            uint32_t GetFirstIndex() const { return m_FirstIndex; }
            uint32_t GetFirstVertex() const { return m_FirstVertex; }

            // Get source mesh resource
            Resources::MeshHandle GetMeshResource() const { return m_MeshResource; }

        private:
            // Source mesh resource (logical resource, not GPU resource)
            Resources::MeshHandle m_MeshResource = nullptr;

            // GPU resources (created in Initialize)
            std::unique_ptr<RHI::IBuffer> m_VertexBuffer;
            std::unique_ptr<RHI::IBuffer> m_IndexBuffer;

            // Geometry data
            uint32_t m_VertexCount = 0;
            uint32_t m_IndexCount = 0;
            uint32_t m_VertexStride = 0;
            uint32_t m_FirstIndex = 0;
            uint32_t m_FirstVertex = 0;

            // Helper method to create GPU buffers
            bool CreateBuffers(RHI::IDevice* device);
        };

    } // namespace Renderer
} // namespace FirstEngine
