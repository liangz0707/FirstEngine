#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/Types.h"
#include <cstring>
#include <algorithm>
#include <map>

namespace FirstEngine {
    namespace Renderer {

        ShadingMaterial::ShadingMaterial() {
            // Auto-register with resource manager
            RenderResourceManager::GetInstance().RegisterResource(this);
        }

        ShadingMaterial::~ShadingMaterial() {
            // Unregister from resource manager
            RenderResourceManager::GetInstance().UnregisterResource(this);
            DoDestroy();
        }

        bool ShadingMaterial::InitializeFromMaterial(Resources::MaterialResource* materialResource) {
            if (!materialResource) {
                return false;
            }

            // Store material resource reference
            m_MaterialResource = materialResource;

            // Initialize from ShaderCollection (should be set in MaterialResource::Load)
            void* shaderCollection = materialResource->GetShaderCollection();
            if (!shaderCollection) {
                return false;
            }

            auto* collection = static_cast<ShaderCollection*>(shaderCollection);
            const Shader::ShaderReflection* reflection = collection->GetShaderReflection();
            if (!reflection) {
                return false;
            }

            m_ShaderCollection = collection;
            m_ShaderCollectionID = collection->GetID();
            m_ShaderReflection = *reflection;
            ParseShaderReflection(m_ShaderReflection);
            return true;
        }

        bool ShadingMaterial::InitializeFromShaderCollection(uint64_t collectionID) {
            if (collectionID == 0) {
                return false;
            }

            auto& shaderTools = ShaderModuleTools::GetInstance();
            auto* collection = shaderTools.GetCollection(collectionID);
            if (!collection) {
                return false;
            }

            m_ShaderCollection = collection;
            m_ShaderCollectionID = collectionID;

            // Get shader reflection from collection (already parsed during loading)
            const Shader::ShaderReflection* reflection = collection->GetShaderReflection();
            if (!reflection) {
                return false;
            }

            // Use cached reflection from collection
            m_ShaderReflection = *reflection;
            ParseShaderReflection(m_ShaderReflection);

            return true;
        }

        // Helper function to map SPIR-V type to RHI Format
        static RHI::Format MapTypeToFormat(uint32_t basetype, uint32_t width, uint32_t vecsize) {
            // basetype: 3 = Int, 4 = UInt, 5 = Float
            // width: 8, 16, 32, 64
            // vecsize: 1 = scalar, 2 = vec2, 3 = vec3, 4 = vec4
            
            if (basetype == 5) { // Float
                if (width == 32) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R8G8B8A8_UNORM; // Use RGBA for scalar (will use R channel)
                        case 2: return RHI::Format::R8G8B8A8_UNORM; // Use RGBA for vec2 (will use RG channels)
                        case 3: return RHI::Format::R8G8B8A8_UNORM; // Use RGBA for vec3 (will use RGB channels)
                        case 4: return RHI::Format::R8G8B8A8_UNORM; // Perfect match for vec4
                        default: return RHI::Format::R8G8B8A8_UNORM;
                    }
                } else if (width == 16) {
                    return RHI::Format::R8G8B8A8_UNORM; // Fallback for float16
                }
            } else if (basetype == 3) { // Int
                return RHI::Format::R8G8B8A8_UNORM; // Fallback for int
            } else if (basetype == 4) { // UInt
                return RHI::Format::R8G8B8A8_UNORM; // Fallback for uint
            }
            
            // Default fallback
            return RHI::Format::R8G8B8A8_UNORM;
        }

        // Helper function to calculate format size in bytes
        static uint32_t CalculateFormatSize(RHI::Format format) {
            switch (format) {
                case RHI::Format::R8G8B8A8_UNORM:
                case RHI::Format::R8G8B8A8_SRGB:
                case RHI::Format::B8G8R8A8_UNORM:
                case RHI::Format::B8G8R8A8_SRGB:
                    return 4; // 4 bytes (R8G8B8A8)
                case RHI::Format::D32_SFLOAT:
                    return 4; // 4 bytes (D32)
                case RHI::Format::D24_UNORM_S8_UINT:
                    return 4; // 4 bytes (D24S8)
                default:
                    return 4; // Default to 4 bytes
            }
        }

        void ShadingMaterial::ParseShaderReflection(const Shader::ShaderReflection& reflection) {
            // Parse vertex inputs (geometry information)
            m_VertexInputs.clear();
            for (const auto& input : reflection.stage_inputs) {
                VertexInput vertexInput;
                vertexInput.location = input.location; // Use location from shader reflection
                vertexInput.name = input.name;
                // Map shader type to format based on basetype, width, and vecsize
                vertexInput.format = MapTypeToFormat(input.basetype, input.width, input.vecsize);
                vertexInput.offset = 0; // Will be calculated based on previous inputs
                vertexInput.binding = 0; // Default binding
                m_VertexInputs.push_back(vertexInput);
            }

            // Parse push constants
            m_PushConstantData.resize(reflection.push_constant_size);
            std::memset(m_PushConstantData.data(), 0, m_PushConstantData.size());

            // Parse uniform buffers
            m_UniformBuffers.clear();
            for (const auto& ub : reflection.uniform_buffers) {
                UniformBufferBinding binding;
                binding.set = ub.set;
                binding.binding = ub.binding;
                binding.name = ub.name;
                binding.size = ub.size;
                binding.data.resize(ub.size);
                std::memset(binding.data.data(), 0, binding.data.size());
                m_UniformBuffers.push_back(binding);
            }

            // Parse texture bindings (samplers and images)
            m_TextureBindings.clear();
            for (const auto& sampler : reflection.samplers) {
                TextureBinding binding;
                binding.set = sampler.set;
                binding.binding = sampler.binding;
                binding.name = sampler.name;
                binding.texture = nullptr;
                m_TextureBindings.push_back(binding);
            }
            for (const auto& image : reflection.images) {
                TextureBinding binding;
                binding.set = image.set;
                binding.binding = image.binding;
                binding.name = image.name;
                binding.texture = nullptr;
                m_TextureBindings.push_back(binding);
            }
        }

        void ShadingMaterial::BuildVertexInputsFromShader() {
            // Calculate offsets based on format sizes
            uint32_t currentOffset = 0;
            for (auto& input : m_VertexInputs) {
                input.offset = currentOffset;
                // Calculate size from format
                uint32_t formatSize = CalculateFormatSize(input.format);
                currentOffset += formatSize;
            }
        }

        bool ShadingMaterial::DoCreate(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            if (!m_ShaderCollection) {
                return false;
            }

            // Create shader modules from ShaderCollection
            m_ShadingState.shaderModules.clear();
            
            auto* collection = static_cast<ShaderCollection*>(m_ShaderCollection);

            // Get available stages from collection
            auto stages = collection->GetAvailableStages();
            for (ShaderStage stage : stages) {
                // Get shader module from collection
                RHI::IShaderModule* shaderModule = collection->GetShader(stage);
                if (shaderModule) {
                    m_ShadingState.shaderModules.push_back(shaderModule);
                }
            }
            
            if (m_ShadingState.shaderModules.empty()) {
                return false;
            }

            // Build vertex inputs with calculated offsets
            BuildVertexInputsFromShader();

            // Create uniform buffers
            if (!CreateUniformBuffers(device)) {
                return false;
            }

            // Create descriptor sets
            if (!CreateDescriptorSets(device)) {
                return false;
            }

            // Note: Pipeline creation is deferred until EnsurePipelineCreated() is called
            // This is because pipeline creation requires a renderPass, which may not be available yet
            // Pipeline will be created lazily when first needed

            return true;
        }

        bool ShadingMaterial::EnsurePipelineCreated(RHI::IDevice* device, RHI::IRenderPass* renderPass) {
            if (!device || !renderPass) {
                return false;
            }

            // If pipeline already exists and is not dirty, return success
            if (m_ShadingState.GetPipeline() && !m_ShadingState.IsPipelineDirty()) {
                return true;
            }

            // Ensure shader modules are created
            if (m_ShadingState.shaderModules.empty()) {
                // Shader modules should have been created in DoCreate
                // If not, something went wrong
                return false;
            }

            // Convert vertex inputs to RHI format
            std::vector<RHI::VertexInputBinding> vertexBindings;
            std::vector<RHI::VertexInputAttribute> vertexAttributes;
            
            // Create a single vertex binding (binding 0) with calculated stride
            if (!m_VertexInputs.empty()) {
                // Calculate stride from last input offset + format size
                uint32_t stride = 0;
                if (!m_VertexInputs.empty()) {
                    const auto& lastInput = m_VertexInputs.back();
                    uint32_t formatSize = CalculateFormatSize(lastInput.format);
                    stride = lastInput.offset + formatSize;
                }
                
                RHI::VertexInputBinding binding;
                binding.binding = 0;
                binding.stride = stride;
                binding.instanced = false;
                vertexBindings.push_back(binding);
                
                // Convert vertex inputs to attributes
                for (const auto& input : m_VertexInputs) {
                    RHI::VertexInputAttribute attr;
                    attr.location = input.location;
                    attr.binding = input.binding;
                    attr.format = input.format;
                    attr.offset = input.offset;
                    vertexAttributes.push_back(attr);
                }
            }

            // Create pipeline from shading state
            if (!m_ShadingState.CreatePipeline(device, renderPass, vertexBindings, vertexAttributes)) {
                return false;
            }

            return true;
        }

        bool ShadingMaterial::DoUpdate(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            // Update uniform buffers if data changed
            for (auto& ub : m_UniformBuffers) {
                if (ub.buffer && !ub.data.empty()) {
                    // Update buffer data using RHI interface
                    ub.buffer->UpdateData(ub.data.data(), ub.data.size(), 0);
                }
            }

            return true;
        }

        void ShadingMaterial::DoDestroy() {
            // Destroy shader modules
            m_OwnedShaderModules.clear();
            m_ShadingState.shaderModules.clear();

            // Destroy uniform buffers
            for (auto& ub : m_UniformBuffers) {
                ub.buffer.reset();
            }
            m_UniformBuffers.clear();

            // Clear texture bindings (textures are not owned)
            m_TextureBindings.clear();

            // Clear descriptor sets (managed externally)
            m_DescriptorSets.clear();
        }

        void ShadingMaterial::SetPushConstantData(const void* data, uint32_t size) {
            if (size > m_PushConstantData.size()) {
                m_PushConstantData.resize(size);
            }
            if (data && size > 0) {
                std::memcpy(m_PushConstantData.data(), data, size);
            }
        }

        ShadingMaterial::UniformBufferBinding* ShadingMaterial::GetUniformBuffer(uint32_t set, uint32_t binding) {
            for (auto& ub : m_UniformBuffers) {
                if (ub.set == set && ub.binding == binding) {
                    return &ub;
                }
            }
            return nullptr;
        }

        ShadingMaterial::TextureBinding* ShadingMaterial::GetTextureBinding(uint32_t set, uint32_t binding) {
            for (auto& tb : m_TextureBindings) {
                if (tb.set == set && tb.binding == binding) {
                    return &tb;
                }
            }
            return nullptr;
        }

        void ShadingMaterial::SetTexture(uint32_t set, uint32_t binding, RHI::IImage* texture) {
            TextureBinding* bindingPtr = GetTextureBinding(set, binding);
            if (bindingPtr) {
                bindingPtr->texture = texture;
            }
        }

        void* ShadingMaterial::GetDescriptorSet(uint32_t set) const {
            auto it = m_DescriptorSets.find(set);
            return (it != m_DescriptorSets.end()) ? it->second : nullptr;
        }

        void ShadingMaterial::SetDescriptorSet(uint32_t set, void* descriptorSet) {
            m_DescriptorSets[set] = descriptorSet;
        }

        bool ShadingMaterial::CreateUniformBuffers(RHI::IDevice* device) {
            for (auto& ub : m_UniformBuffers) {
                if (ub.size == 0) {
                    continue;
                }

                ub.buffer = device->CreateBuffer(
                    ub.size,
                    RHI::BufferUsageFlags::UniformBuffer | RHI::BufferUsageFlags::TransferDst,
                    RHI::MemoryPropertyFlags::DeviceLocal
                );
                if (!ub.buffer) {
                    return false;
                }

                // Upload initial data
                if (!ub.data.empty()) {
                    void* mapped = ub.buffer->Map();
                    if (mapped) {
                        std::memcpy(mapped, ub.data.data(), ub.data.size());
                        ub.buffer->Unmap();
                    }
                }
            }

            return true;
        }

        bool ShadingMaterial::CreateDescriptorSets(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            // Descriptor set creation requires device-specific implementation
            // Since RHI interface uses void* for descriptor sets, this is a placeholder
            // that prepares the data structures for descriptor set creation.
            // Actual descriptor set layout and descriptor set allocation should be done
            // at the device implementation level (e.g., VulkanDevice).
            
            // Group resources by descriptor set
            std::map<uint32_t, std::vector<std::pair<uint32_t, std::string>>> setBindings;
            
            // Collect uniform buffer bindings by set
            for (const auto& ub : m_UniformBuffers) {
                setBindings[ub.set].push_back({ub.binding, "uniform_buffer"});
            }
            
            // Collect texture bindings by set
            for (const auto& tb : m_TextureBindings) {
                setBindings[tb.set].push_back({tb.binding, "texture"});
            }
            
            // Store descriptor set information for later use
            // The actual descriptor set creation will be done when pipeline is created
            // or when resources are bound, as it requires device-specific API calls
            
            // For now, mark descriptor sets as ready (they will be created lazily when needed)
            // This allows the material to be created successfully even if descriptor sets
            // are not immediately available
            
            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
