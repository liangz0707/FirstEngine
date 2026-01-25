#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/MaterialParameter.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace Renderer {
        class ShadingMaterial;
    }
    
    namespace Resources {

        // Forward declaration
        class ResourceManager;

        // Material resource implementation
        // Implements both IMaterial (resource itself) and IResourceProvider (loading methods)
        class FE_RESOURCES_API MaterialResource : public IMaterial, public IResourceProvider {
        public:
            MaterialResource();
            ~MaterialResource() override;

            // IResourceProvider interface
            bool IsFormatSupported(const std::string& filepath) const override;
            std::vector<std::string> GetSupportedFormats() const override;
            ResourceLoadResult Load(ResourceID id) override;
            void LoadDependencies() override;

            // IMaterial interface
            const std::string& GetShaderName() const override { return m_ShaderName; }
            void SetShaderName(const std::string& name) override { m_ShaderName = name; }

            void SetTexture(const std::string& slot, TextureHandle texture) override;
            TextureHandle GetTexture(const std::string& slot) const override;

            // Get parameter data as serialized byte stream (for shader upload)
            // Note: This method needs to be called after BuildParameterData() to ensure data is up to date
            const void* GetParameterData() const override;
            uint32_t GetParameterDataSize() const override;

            // Set material parameters (generic key-value pairs)
            void SetParameter(const std::string& name, const MaterialParameterValue& value);
            void SetParameter(const std::string& name, float value);
            void SetParameter(const std::string& name, const glm::vec2& value);
            void SetParameter(const std::string& name, const glm::vec3& value);
            void SetParameter(const std::string& name, const glm::vec4& value);
            void SetParameter(const std::string& name, int32_t value);
            void SetParameter(const std::string& name, bool value);
            void SetParameter(const std::string& name, const glm::mat3& value);
            void SetParameter(const std::string& name, const glm::mat4& value);

            // Get parameter by name
            const MaterialParameterValue* GetParameter(const std::string& name) const;
            
            // Get all parameters
            const MaterialParameters& GetParameters() const { return m_Parameters; }

            // Build parameter data buffer (serializes all parameters into byte stream)
            // Should be called after modifying parameters to update the parameter data buffer
            void BuildParameterData();

            // Set texture by ResourceID (path handling is hidden in ResourceManager)
            void SetTextureID(const std::string& slot, ResourceID textureID);
            ResourceID GetTextureID(const std::string& slot) const;

            // Save resource to XML file
            bool Save(const std::string& xmlFilePath) const;

            // IResource interface
            const ResourceMetadata& GetMetadata() const override { return m_Metadata; }
            ResourceMetadata& GetMetadata() override { return m_Metadata; }
            void AddRef() override { m_Metadata.refCount++; }
            void Release() override { m_Metadata.refCount--; }
            uint32_t GetRefCount() const override { return m_Metadata.refCount; }

            // DEPRECATED: ShadingMaterial is now owned by Component, not MaterialResource
            // These methods are kept for backward compatibility but should not be used
            // TODO: Remove these methods in future versions
            [[deprecated("ShadingMaterial is now owned by Component. Use Component::GetShadingMaterial() instead.")]]
            bool CreateShadingMaterial();
            
            [[deprecated("ShadingMaterial is now owned by Component. Use Component::GetShadingMaterial() instead.")]]
            bool IsShadingMaterialReady() const;
            
            [[deprecated("ShadingMaterial is now owned by Component. Use Component::GetShadingMaterial() instead.")]]
            struct RenderData {
                void* shadingMaterial = nullptr; // ShadingMaterial* cast to void*
                void* pipeline = nullptr; // IPipeline* cast to void*
                void* descriptorSet = nullptr; // Descriptor set
                std::string materialName;
                void* image = nullptr; // RHI::IImage* cast to void* (for texture resources)
            };
            [[deprecated("ShadingMaterial is now owned by Component. Use Component::GetShadingMaterial() instead.")]]
            bool GetRenderData(RenderData& outData) const;
            
            // Helper: Set textures from MaterialResource to ShadingMaterial
            // This is used by Component to set textures after ShadingMaterial is created
            // shadingMaterial: The ShadingMaterial to set textures to
            void SetTexturesToShadingMaterial(Renderer::ShadingMaterial* shadingMaterial) const;
            
            // Get all textures (for Component to access)
            const std::unordered_map<std::string, TextureHandle>& GetTextures() const { return m_Textures; }

            // Get ShaderCollection (for accessing shader modules)
            // Returns nullptr if not set
            void* GetShaderCollection() const { return m_ShaderCollection; }
            
            // Set ShaderCollection by ID (looks up from ShaderCollectionsTools)
            void SetShaderCollectionID(uint64_t collectionID);

        private:
            ResourceMetadata m_Metadata;
            std::string m_ShaderName; // Shader name (used to find ShaderCollection)
            
            // ShaderCollection (stored as void* to avoid circular dependency)
            // Points to Renderer::ShaderCollection*
            void* m_ShaderCollection = nullptr;
            uint64_t m_ShaderCollectionID = 0;
            
            std::unordered_map<std::string, TextureHandle> m_Textures; // Texture handles (loaded textures)
            std::unordered_map<std::string, ResourceID> m_TextureIDs; // Texture IDs (for dependencies)
            
            // Material parameters (key-value pairs)
            MaterialParameters m_Parameters;
            
            // Serialized parameter data (for shader upload)
            std::vector<uint8_t> m_ParameterData;
            bool m_ParameterDataDirty = true; // Flag to indicate if parameter data needs rebuilding
            
            // DEPRECATED: ShadingMaterial is now owned by Component, not MaterialResource
            // This is kept for backward compatibility
            void* m_ShadingMaterial = nullptr;
        };

    } // namespace Resources
} // namespace FirstEngine
