#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/ShadingState.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/Core/MathTypes.h"
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
        class MaterialDescriptorManager;
        class RenderParameterCollector;
        class RenderGeometry;
        struct RenderParameterValue;
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

            // Get device (stored from DoCreate for cleanup)
            RHI::IDevice* GetDevice() const { return m_Device; }

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
                RHI::DescriptorType descriptorType = RHI::DescriptorType::CombinedImageSampler; // Descriptor type from shader reflection
            };
            TextureBinding* GetTextureBinding(uint32_t set, uint32_t binding);
            const std::vector<TextureBinding>& GetTextureBindings() const { return m_TextureBindings; }

            // Set texture for a binding
            void SetTexture(uint32_t set, uint32_t binding, RHI::IImage* texture);

            // Descriptor sets (accessed through MaterialDescriptorManager)
            // Returns descriptor set handle for the given set index
            void* GetDescriptorSet(uint32_t set) const;
            
            // Get descriptor set layout (for pipeline creation)
            // Returns descriptor set layout handle for the given set index
            void* GetDescriptorSetLayout(uint32_t set) const;
            
            // Get all descriptor set layouts (for pipeline creation)
            // Returns all descriptor set layout handles in order
            std::vector<void*> GetAllDescriptorSetLayouts() const;

            // Get shader reflection data
            const Shader::ShaderReflection& GetShaderReflection() const { return m_ShaderReflection; }

            // Get source material resource
            Resources::MaterialResource* GetMaterialResource() const { return m_MaterialResource; }

            // Ensure pipeline is created (lazy creation)
            // This will create the pipeline if it doesn't exist yet
            // renderPass: The render pass to use for pipeline creation
            // Returns true if pipeline is ready, false otherwise
            bool EnsurePipelineCreated(RHI::IDevice* device, RHI::IRenderPass* renderPass);

            // ============================================================================
            // Per-frame update interfaces
            // ============================================================================

            // Update texture for a specific binding (per-frame)
            // set: Descriptor set index
            // binding: Binding index within the set
            // texture: Texture to bind (can be nullptr to unbind)
            // Returns true if binding was found and updated
            bool UpdateTexture(uint32_t set, uint32_t binding, RHI::IImage* texture);

            // Update texture by shader variable name (per-frame)
            // name: Shader variable name (e.g., "albedoMap", "normalMap")
            // texture: Texture to bind (can be nullptr to unbind)
            // Returns true if texture binding was found and updated
            bool UpdateTextureByName(const std::string& name, RHI::IImage* texture);

            // Structure for per-frame uniform buffer updates
            struct UniformBufferUpdate {
                uint32_t set = 0;
                uint32_t binding = 0;
                std::string name; // Optional: shader variable name
                const void* data = nullptr; // Data to update
                uint32_t size = 0; // Size of data in bytes
                uint32_t offset = 0; // Offset within the buffer (default: 0)
            };

            // Update uniform buffer data (per-frame)
            // update: Update structure containing set, binding, data, size, and offset
            // Returns true if buffer was found and updated successfully
            bool UpdateUniformBuffer(const UniformBufferUpdate& update);

            // Update uniform buffer by shader variable name (per-frame)
            // name: Shader variable name (e.g., "MaterialParams", "LightData")
            // data: Data to update
            // size: Size of data in bytes
            // offset: Offset within the buffer (default: 0)
            // Returns true if buffer was found and updated successfully
            bool UpdateUniformBufferByName(const std::string& name, const void* data, uint32_t size, uint32_t offset = 0);

            // Structure for per-frame push constant updates
            struct PushConstantUpdate {
                const void* data = nullptr; // Data to update
                uint32_t size = 0; // Size of data in bytes
                uint32_t offset = 0; // Offset within push constant buffer (default: 0)
            };

            // Update push constant data (per-frame)
            // update: Update structure containing data, size, and offset
            // Returns true if update was successful
            bool UpdatePushConstant(const PushConstantUpdate& update);

            // ============================================================================
            // Geometry validation interface
            // ============================================================================

            // Check if vertex inputs match the geometry data
            // geometry: RenderGeometry to check against
            // Returns true if vertex inputs are compatible with geometry
            bool ValidateVertexInputs(const RenderGeometry* geometry) const;

            // ============================================================================
            // Render parameter management interface
            // ============================================================================

            // Render parameter value type - uses encapsulated math types
            // This is a simplified version that delegates to RenderParameterCollector
            // For actual parameter collection, use RenderParameterCollector
            struct RenderParameterValue {
                enum class Type : uint32_t {
                    Texture = 0,      // RHI::IImage*
                    Float = 1,
                    Vec2 = 2,
                    Vec3 = 3,
                    Vec4 = 4,
                    Int = 5,
                    Bool = 6,
                    Mat3 = 7,
                    Mat4 = 8,
                    RawData = 9,      // Raw byte data for uniform buffers
                    PushConstant = 10 // Raw byte data for push constants (with offset)
                };

                Type type;
                std::vector<uint8_t> data; // Raw data storage
                uint32_t m_Offset = 0; // Offset for push constants

                // Constructors - use encapsulated math types
                RenderParameterValue() : type(Type::Float), data(sizeof(float)), m_Offset(0) {}
                RenderParameterValue(RHI::IImage* texture);
                RenderParameterValue(float value);
                RenderParameterValue(const Core::Vec2& value);
                RenderParameterValue(const Core::Vec3& value);
                RenderParameterValue(const Core::Vec4& value);
                RenderParameterValue(int32_t value);
                RenderParameterValue(bool value);
                RenderParameterValue(const Core::Mat3& value);
                RenderParameterValue(const Core::Mat4& value);
                RenderParameterValue(const void* rawData, uint32_t size); // Raw data
                RenderParameterValue(const void* rawData, uint32_t size, uint32_t offset); // Push constant data with offset

                // Getters - return encapsulated math types
                RHI::IImage* GetTexture() const;
                uint32_t GetOffset() const { return m_Offset; } // For push constants
                float GetFloat() const;
                Core::Vec2 GetVec2() const;
                Core::Vec3 GetVec3() const;
                Core::Vec4 GetVec4() const;
                int32_t GetInt() const;
                bool GetBool() const;
                Core::Mat3 GetMat3() const;
                Core::Mat4 GetMat4() const;
                const void* GetRawData() const { return data.data(); }
                uint32_t GetRawDataSize() const { return static_cast<uint32_t>(data.size()); }
            };

            // Render parameters - key-value pairs
            using RenderParameters = std::unordered_map<std::string, RenderParameterValue>;

            // Set render parameter by key-value pair
            // key: Parameter name (e.g., "albedoMap" for texture, "MaterialParams" for uniform buffer)
            // value: Parameter value (texture, float, vec, etc.)
            // Returns true if parameter was set successfully
            bool SetRenderParameter(const std::string& key, const RenderParameterValue& value);

            // Get render parameter by key
            // Returns nullptr if parameter not found
            const RenderParameterValue* GetRenderParameter(const std::string& key) const;

            // Get all render parameters
            const RenderParameters& GetRenderParameters() const { return m_RenderParameters; }

            // Clear all render parameters
            void ClearRenderParameters() { m_RenderParameters.clear(); }

            // Update render parameters per frame (called before rendering)
            // This method applies all pending render parameters to the material
            // NOTE: This only updates CPU-side data, not GPU buffers
            // Returns true if update was successful
            bool UpdateRenderParameters();

            // Apply parameters from RenderParameterCollector
            // collector: Parameter collector containing parameters from various sources
            // This sets parameters in CPU-side storage (m_RenderParameters)
            void ApplyParameters(const RenderParameterCollector& collector);

            // Flush parameters to GPU buffers (called during DoUpdate)
            // This transfers CPU-side parameter data to actual GPU buffers
            // Should be called after UpdateRenderParameters and before rendering
            bool FlushParametersToGPU(RHI::IDevice* device);

        private:
            // Device reference (for cleanup)
            RHI::IDevice* m_Device = nullptr;

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
            // These store CPU-side data and GPU buffer references
            std::vector<UniformBufferBinding> m_UniformBuffers;

            // Texture bindings (indexed by set and binding)
            // These store texture references (not owned)
            std::vector<TextureBinding> m_TextureBindings;

            // Descriptor manager - handles all device-specific descriptor operations
            std::unique_ptr<MaterialDescriptorManager> m_DescriptorManager;

            // Owned shader modules (keep them alive)
            std::vector<std::unique_ptr<RHI::IShaderModule>> m_OwnedShaderModules;

            // Per-frame render parameters (key-value pairs)
            // Updated via SetRenderParameter, applied in UpdateRenderParameters
            RenderParameters m_RenderParameters;

            // Helper methods
            void ParseShaderReflection(const Shader::ShaderReflection& reflection);
            void BuildVertexInputsFromShader();
            bool CreateUniformBuffers(RHI::IDevice* device);
            
            // Initialize textures and buffers from MaterialResource
            void InitializeFromMaterialResource(Resources::MaterialResource* materialResource);

            // Internal helper: Apply a single render parameter to CPU-side data
            // includeTextures: If true, also update texture bindings; if false, skip textures
            // Returns true if parameter was applied successfully
            bool ApplyRenderParameter(const std::string& key, const RenderParameterValue& value, bool includeTextures);
        };

    } // namespace Renderer
} // namespace FirstEngine
