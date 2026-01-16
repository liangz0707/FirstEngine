#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/ShadingState.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/Shader/ShaderCompiler.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/Types.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace RHI {
        class IDevice;
        class IRenderPass;
    }

    namespace Resources {
        class MaterialResource;
    }

    namespace Renderer {

        // ShadingMaterial - records ShadingState and all Shader parameters
        // Includes PushConstants, Uniform Buffers, Textures, etc. parsed from shader
        class FE_RENDERER_API ShadingMaterial : public IRenderResource {
        public:
            ShadingMaterial();
            ~ShadingMaterial() override;

            // Initialize from MaterialResource (handles all setup internally)
            // materialResource: MaterialResource to initialize from
            // Returns true if successful, false otherwise
            bool InitializeFromMaterial(Resources::MaterialResource* materialResource);

            // Initialize from ShaderCollection (uses ShaderModuleTools)
            // collectionID: ID of the ShaderCollection to use
            // Returns true if successful, false otherwise
            bool InitializeFromShaderCollection(uint64_t collectionID);

            // IRenderResource interface
            bool DoCreate(RHI::IDevice* device) override;
            bool DoUpdate(RHI::IDevice* device) override;
            void DoDestroy() override;

            // ShadingState access
            ShadingState& GetShadingState() { return m_ShadingState; }
            const ShadingState& GetShadingState() const { return m_ShadingState; }

            // Geometry information (parsed from shader stage inputs)
            struct VertexInput {
                uint32_t location;
                std::string name;
                RHI::Format format;
                uint32_t offset;
                uint32_t binding;
            };
            const std::vector<VertexInput>& GetVertexInputs() const { return m_VertexInputs; }

            // Push constant data
            void SetPushConstantData(const void* data, uint32_t size);
            const void* GetPushConstantData() const { return m_PushConstantData.data(); }
            uint32_t GetPushConstantSize() const { return static_cast<uint32_t>(m_PushConstantData.size()); }

            // Uniform buffers (by binding/set)
            struct UniformBufferBinding {
                uint32_t set;
                uint32_t binding;
                std::string name;
                uint32_t size;
                std::unique_ptr<RHI::IBuffer> buffer; // GPU buffer
                std::vector<uint8_t> data; // CPU data
            };
            UniformBufferBinding* GetUniformBuffer(uint32_t set, uint32_t binding);
            const std::vector<UniformBufferBinding>& GetUniformBuffers() const { return m_UniformBuffers; }

            // Textures/Samplers (by binding/set)
            struct TextureBinding {
                uint32_t set;
                uint32_t binding;
                std::string name;
                RHI::IImage* texture = nullptr; // Texture reference (not owned)
            };
            TextureBinding* GetTextureBinding(uint32_t set, uint32_t binding);
            const std::vector<TextureBinding>& GetTextureBindings() const { return m_TextureBindings; }

            // Set texture for a binding
            void SetTexture(uint32_t set, uint32_t binding, RHI::IImage* texture);

            // Descriptor sets (created from uniform buffers and textures)
            void* GetDescriptorSet(uint32_t set) const;
            void SetDescriptorSet(uint32_t set, void* descriptorSet);

            // Get shader reflection data
            const Shader::ShaderReflection& GetShaderReflection() const { return m_ShaderReflection; }

            // Get source material resource
            Resources::MaterialResource* GetMaterialResource() const { return m_MaterialResource; }

            // Ensure pipeline is created (lazy creation)
            // This will create the pipeline if it doesn't exist yet
            // renderPass: The render pass to use for pipeline creation
            // Returns true if pipeline is ready, false otherwise
            bool EnsurePipelineCreated(RHI::IDevice* device, RHI::IRenderPass* renderPass);

        private:
            // Source material resource (logical resource, not GPU resource)
            Resources::MaterialResource* m_MaterialResource = nullptr;
            // ShaderCollection (stores shader modules and reflection)
            void* m_ShaderCollection = nullptr; // ShaderCollection* cast to void* to avoid circular dependency
            uint64_t m_ShaderCollectionID = 0;

            // Shading state (pipeline state + shaders)
            ShadingState m_ShadingState;

            // Shader reflection data (parsed from shader)
            Shader::ShaderReflection m_ShaderReflection;

            // Geometry information (from shader stage inputs)
            std::vector<VertexInput> m_VertexInputs;

            // Push constant data
            std::vector<uint8_t> m_PushConstantData;

            // Uniform buffers (indexed by set and binding)
            std::vector<UniformBufferBinding> m_UniformBuffers;

            // Texture bindings (indexed by set and binding)
            std::vector<TextureBinding> m_TextureBindings;

            // Descriptor sets (one per set index)
            std::unordered_map<uint32_t, void*> m_DescriptorSets;

            // Owned shader modules (keep them alive)
            std::vector<std::unique_ptr<RHI::IShaderModule>> m_OwnedShaderModules;

            // Helper methods
            void ParseShaderReflection(const Shader::ShaderReflection& reflection);
            void BuildVertexInputsFromShader();
            bool CreateUniformBuffers(RHI::IDevice* device);
            bool CreateDescriptorSets(RHI::IDevice* device);
        };

    } // namespace Renderer
} // namespace FirstEngine
