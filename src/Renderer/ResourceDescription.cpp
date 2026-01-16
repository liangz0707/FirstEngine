#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        // ResourceDescription implementation
        ResourceDescription::ResourceDescription(ResourceType type, const std::string& name)
            : m_Type(type), m_Name(name) {
        }

        // AttachmentResource implementation
        AttachmentResource::AttachmentResource(
            const std::string& name,
            uint32_t width,
            uint32_t height,
            RHI::Format format,
            bool hasDepth
        ) : ResourceDescription(ResourceType::Attachment, name) {
            m_Width = width;
            m_Height = height;
            m_Format = format;
            m_HasDepth = hasDepth;
        }

        // BufferResource implementation
        BufferResource::BufferResource(
            const std::string& name,
            uint64_t size,
            RHI::BufferUsageFlags usage
        ) : ResourceDescription(ResourceType::Buffer, name) {
            m_Size = size;
            m_BufferUsage = usage;
        }

    } // namespace Renderer
} // namespace FirstEngine
