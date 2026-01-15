#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace FirstEngine {
    namespace Renderer {

        // Render mesh - holds RHI buffers and mesh data for rendering
        class FE_RENDERER_API RenderMesh {
        public:
            RenderMesh();
            ~RenderMesh();

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            RenderMesh(const RenderMesh&) = delete;
            RenderMesh& operator=(const RenderMesh&) = delete;

            // Allow move constructor and move assignment operator
            RenderMesh(RenderMesh&&) noexcept = default;
            RenderMesh& operator=(RenderMesh&&) noexcept = default;

            // Initialize from mesh resource
            bool Initialize(RHI::IDevice* device, Resources::MeshHandle meshResource);
            
            // Get RHI resources
            RHI::IBuffer* GetVertexBuffer() const { return m_VertexBuffer.get(); }
            RHI::IBuffer* GetIndexBuffer() const { return m_IndexBuffer.get(); }
            
            // Get mesh data
            uint32_t GetVertexCount() const { return m_VertexCount; }
            uint32_t GetIndexCount() const { return m_IndexCount; }
            uint32_t GetVertexStride() const { return m_VertexStride; }
            bool IsIndexed() const { return m_IndexCount > 0; }

            // Get original mesh resource
            Resources::MeshHandle GetMeshResource() const { return m_MeshResource; }

        private:
            RHI::IDevice* m_Device = nullptr;
            std::unique_ptr<RHI::IBuffer> m_VertexBuffer;
            std::unique_ptr<RHI::IBuffer> m_IndexBuffer;
            Resources::MeshHandle m_MeshResource = nullptr;
            uint32_t m_VertexCount = 0;
            uint32_t m_IndexCount = 0;
            uint32_t m_VertexStride = 0;
        };

        // Render material - holds shader parameters and textures for rendering
        class FE_RENDERER_API RenderMaterial {
        public:
            RenderMaterial();
            ~RenderMaterial();

            // Delete copy constructor and copy assignment operator (contains unique_ptr)
            RenderMaterial(const RenderMaterial&) = delete;
            RenderMaterial& operator=(const RenderMaterial&) = delete;

            // Allow move constructor and move assignment operator
            RenderMaterial(RenderMaterial&&) noexcept = default;
            RenderMaterial& operator=(RenderMaterial&&) noexcept = default;

            // Initialize from material resource
            bool Initialize(RHI::IDevice* device, Resources::MaterialHandle materialResource);

            // Shader name
            void SetShaderName(const std::string& name) { m_ShaderName = name; }
            const std::string& GetShaderName() const { return m_ShaderName; }

            // Texture management
            void SetTexture(const std::string& slot, RHI::IImage* texture);
            RHI::IImage* GetTexture(const std::string& slot) const;
            const std::unordered_map<std::string, RHI::IImage*>& GetTextures() const { return m_Textures; }

            // Common texture slots
            RHI::IImage* GetAlbedoTexture() const { return GetTexture("Albedo"); }
            RHI::IImage* GetNormalTexture() const { return GetTexture("Normal"); }
            RHI::IImage* GetMetallicRoughnessTexture() const { return GetTexture("MetallicRoughness"); }
            RHI::IImage* GetEmissiveTexture() const { return GetTexture("Emissive"); }

            // Shader parameters (uniform buffer data)
            void SetParameterData(const void* data, uint32_t size);
            const void* GetParameterData() const { return m_ParameterData.data(); }
            uint32_t GetParameterDataSize() const { return static_cast<uint32_t>(m_ParameterData.size()); }
            RHI::IBuffer* GetParameterBuffer() const { return m_ParameterBuffer.get(); }

            // Get original material resource
            Resources::MaterialHandle GetMaterialResource() const { return m_MaterialResource; }

        private:
            RHI::IDevice* m_Device = nullptr;
            std::string m_ShaderName;
            std::unordered_map<std::string, RHI::IImage*> m_Textures;
            std::vector<std::unique_ptr<RHI::IImage>> m_OwnedImages; // Store ownership of images
            std::vector<uint8_t> m_ParameterData;
            std::unique_ptr<RHI::IBuffer> m_ParameterBuffer; // Uniform buffer for shader parameters
            Resources::MaterialHandle m_MaterialResource = nullptr;
        };

    } // namespace Renderer
} // namespace FirstEngine
