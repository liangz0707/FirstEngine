#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/Renderer/MaterialDescriptorManager.h"
#include "FirstEngine/Renderer/RenderResourceManager.h"
#include "FirstEngine/Renderer/RenderParameterCollector.h"
#include "FirstEngine/Renderer/ShaderCollectionsTools.h"
#include "FirstEngine/Renderer/ShaderModuleTools.h"
#include "FirstEngine/Renderer/ShaderCollection.h"
#include "FirstEngine/Renderer/RenderGeometry.h"
#include "FirstEngine/Core/MathTypes.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IImage.h"
#include "FirstEngine/RHI/Types.h"
#include <cstring>
#include <algorithm>
#include <map>
#include <iostream>

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

            // Initialize textures and buffers from MaterialResource
            InitializeFromMaterialResource(materialResource);

            return true;
        }

        void ShadingMaterial::InitializeFromMaterialResource(Resources::MaterialResource* materialResource) {
            if (!materialResource) {
                return;
            }

            // Initialize uniform buffers from MaterialResource parameters
            // This initializes the CPU-side data, which will be uploaded to GPU in DoCreate
            const auto& parameters = materialResource->GetParameters();
            
            // Get shader reflection to access uniform buffer member information
            const auto& reflection = m_ShaderReflection;
            
            for (const auto& [paramName, paramValue] : parameters) {
                bool parameterSet = false;
                
                // First, try to match parameter to uniform buffer members using shader reflection
                for (const auto& ubReflection : reflection.uniform_buffers) {
                    // Find the corresponding UniformBufferBinding
                    UniformBufferBinding* ub = GetUniformBuffer(ubReflection.set, ubReflection.binding);
                    if (!ub) {
                        continue;
                    }
                    
                    // Check if parameter name matches uniform buffer name
                    if (ubReflection.name == paramName) {
                        // Parameter name matches buffer name - copy entire buffer data
                        const void* data = paramValue.GetData();
                        uint32_t size = paramValue.GetDataSize();
                        if (data && size > 0 && size <= ub->size) {
                            std::memcpy(ub->data.data(), data, std::min(size, ub->size));
                            parameterSet = true;
                            break;
                        }
                    }
                    
                    // Try to match parameter to uniform buffer members
                    uint32_t currentOffset = 0;
                    for (const auto& member : ubReflection.members) {
                        // Check if parameter name matches member name
                        if (member.name == paramName) {
                            // Calculate member offset (simplified - assumes sequential packing)
                            // Note: In a full implementation, we should use actual member offsets from reflection
                            const void* data = paramValue.GetData();
                            uint32_t size = paramValue.GetDataSize();
                            
                            // Ensure we don't overflow the buffer
                            if (currentOffset + size <= ub->size) {
                                std::memcpy(ub->data.data() + currentOffset, data, size);
                                parameterSet = true;
                                break;
                            }
                        }
                        
                        // Update offset for next member (simplified - should use actual member size from reflection)
                        // For now, we'll use a conservative estimate
                        currentOffset += member.size > 0 ? member.size : 16; // Default to 16 bytes if size unknown
                    }
                    
                    if (parameterSet) {
                        break;
                    }
                }
                
                // Fallback: If not matched to any uniform buffer member, try simple name matching
                if (!parameterSet) {
                    for (auto& ub : m_UniformBuffers) {
                        // Check if parameter name matches uniform buffer name (fuzzy match)
                        if (ub.name == paramName || ub.name.find(paramName) != std::string::npos) {
                            // Update buffer data with parameter value
                            const void* data = paramValue.GetData();
                            uint32_t size = paramValue.GetDataSize();
                            if (data && size > 0 && size <= ub.size) {
                                std::memcpy(ub.data.data(), data, std::min(size, ub.size));
                            }
                            break;
                        }
                    }
                }
            }
            
            // Note: Textures will be initialized after ShadingMaterial is created
            // via MaterialResource::SetTexturesToShadingMaterial() which is called
            // from MaterialResource::GetRenderData() after DoCreate()
            // This is because textures need GPU resources to be created first
        }

        bool ShadingMaterial::InitializeFromShaderCollection(uint64_t collectionID) {
            if (collectionID == 0) {
                return false;
            }

            auto& collectionsTools = ShaderCollectionsTools::GetInstance();
            auto* collection = collectionsTools.GetCollection(collectionID);
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

        // ============================================================================
        // 辅助函数：将 SPIR-V 类型映射为 RHI Format
        // ============================================================================
        // 
        // 参数说明：
        //   - basetype: SPIRType 基础类型
        //     * 3 = Int (有符号整数)
        //     * 4 = UInt (无符号整数)
        //     * 5 = Float (浮点数)
        //   - width: 类型宽度（位数）
        //     * 8 = 8 位 (int8, uint8, float8)
        //     * 16 = 16 位 (int16, uint16, float16/half)
        //     * 32 = 32 位 (int32, uint32, float32)
        //     * 64 = 64 位 (int64, uint64, float64/double)
        //   - vecsize: 向量大小
        //     * 1 = scalar (标量，如 float, int)
        //     * 2 = vec2 (2 分量向量，如 vec2, float2)
        //     * 3 = vec3 (3 分量向量，如 vec3, float3)
        //     * 4 = vec4 (4 分量向量，如 vec4, float4)
        //
        // 返回值：
        //   RHI::Format - 对应的顶点属性格式
        //
        // 格式的用处：
        //   - 确定顶点属性在内存中的布局和大小
        //   - 用于创建 VkVertexInputAttributeDescription
        //   - 影响顶点数据的读取和解释方式
        //   - 例如：vec3 float32 -> 12 字节，vec4 float32 -> 16 字节
        //
        // 注意：
        //   当前实现使用 R8G8B8A8_UNORM 作为通用格式（4 字节），
        //   对于 vec3 (12 字节) 等非 4 字节对齐的类型，可能需要调整
        // ============================================================================
        static RHI::Format MapTypeToFormat(uint32_t basetype, uint32_t width, uint32_t vecsize) {
            
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

        // ============================================================================
        // 辅助函数：计算格式的字节大小
        // ============================================================================
        // 
        // 参数说明：
        //   - format: RHI::Format 枚举值，表示顶点属性的数据格式
        //
        // 返回值：
        //   uint32_t - 格式占用的字节数
        //
        // 格式说明：
        //   - R8G8B8A8_UNORM: 4 个 8 位无符号归一化通道 (R, G, B, A) = 4 字节
        //   - R8G8B8A8_SRGB: 4 个 8 位 sRGB 通道 (R, G, B, A) = 4 字节
        //   - B8G8R8A8_UNORM: 4 个 8 位无符号归一化通道 (B, G, R, A) = 4 字节
        //   - B8G8R8A8_SRGB: 4 个 8 位 sRGB 通道 (B, G, R, A) = 4 字节
        //   - D32_SFLOAT: 32 位深度浮点数 = 4 字节
        //   - D24_UNORM_S8_UINT: 24 位深度 + 8 位模板 = 4 字节（打包）
        //
        // 尺寸的用处：
        //   - 用于计算顶点属性在缓冲区中的偏移量（offset）
        //   - 用于计算顶点数据的 stride（步长）
        //   - 确保 GPU 能正确读取和解释顶点数据
        //   - 影响内存对齐和性能（某些格式可能需要特定对齐）
        //
        // 注意：
        //   - 格式尺寸必须与实际顶点数据布局匹配
        //   - 某些格式可能需要考虑对齐要求（如 16 字节对齐）
        //   - 当前实现中，所有格式都返回 4 字节（简化实现）
        //     实际项目中可能需要支持更多格式（如 R32G32B32_SFLOAT = 12 字节）
        // ============================================================================
        static uint32_t CalculateFormatSize(RHI::Format format) {
            switch (format) {
                case RHI::Format::R8G8B8A8_UNORM:  // 4 字节：RGBA 各 8 位无符号归一化
                case RHI::Format::R8G8B8A8_SRGB:   // 4 字节：RGBA 各 8 位 sRGB
                case RHI::Format::B8G8R8A8_UNORM:  // 4 字节：BGRA 各 8 位无符号归一化
                case RHI::Format::B8G8R8A8_SRGB:   // 4 字节：BGRA 各 8 位 sRGB
                    return 4;
                case RHI::Format::D32_SFLOAT:       // 4 字节：32 位深度浮点数
                    return 4;
                case RHI::Format::D24_UNORM_S8_UINT: // 4 字节：24 位深度 + 8 位模板（打包）
                    return 4;
                default:
                    return 4; // 默认 4 字节（简化实现）
            }
        }

        void ShadingMaterial::ParseShaderReflection(const Shader::ShaderReflection& reflection) {
            // ============================================================================
            // 解析 Shader Reflection 数据，将 shader 中的资源声明转换为渲染管线配置
            // ============================================================================
            // 
            // Shader Reflection 是从编译后的 SPIR-V 代码中提取的元数据信息，
            // 包含了 shader 中声明的所有输入、输出、uniform buffer、纹理等资源。
            // 
            // 对应 Shader 语法示例：
            // - Vertex Input:    layout(location = 0) in vec3 inPosition;
            // - Uniform Buffer:  layout(set = 0, binding = 0) uniform UniformBuffer { ... };
            // - Sampler:         layout(set = 0, binding = 1) uniform sampler2D texSampler;
            // - Image:           layout(set = 0, binding = 2) uniform texture2D texImage;
            // - Push Constant:   layout(push_constant) uniform PushConstants { ... };
            // ============================================================================

            // ----------------------------------------------------------------------------
            // 1. 解析 Vertex Inputs (顶点输入属性)
            // ----------------------------------------------------------------------------
            // 对应 Shader 语法：
            //   layout(location = 0) in vec3 inPosition;
            //   layout(location = 1) in vec2 inTexCoord;
            //   layout(location = 2) in vec3 inNormal;
            //
            // 参数说明：
            //   - location: 顶点属性的位置索引，对应 shader 中的 layout(location = N)
            //     用于在顶点缓冲区中标识不同的属性（位置、法线、纹理坐标等）
            //   - name: 属性在 shader 中的变量名（如 "inPosition", "inTexCoord"）
            //   - format: 顶点属性的数据格式（如 R8G8B8A8_UNORM 表示 4 字节 RGBA）
            //     格式决定了如何从内存中读取和解释顶点数据
            //   - offset: 该属性在顶点缓冲区中的字节偏移量
            //     用于计算属性在顶点数据中的起始位置（在 BuildVertexInputsFromShader 中计算）
            //   - binding: 顶点缓冲区的绑定索引（通常为 0，表示使用第一个顶点缓冲区）
            //
            // 格式和尺寸的用处：
            //   - format 决定了每个属性占用的字节数（如 R8G8B8A8_UNORM = 4 字节）
            //   - offset 用于在顶点缓冲区中正确定位每个属性的数据
            //   - 所有属性的 offset + format 尺寸 = 顶点数据的 stride（步长）
            //   - 这些信息用于创建 VkVertexInputAttributeDescription 和 VkVertexInputBindingDescription
            // ----------------------------------------------------------------------------
            m_VertexInputs.clear();
            for (const auto& input : reflection.stage_inputs) {
                VertexInput vertexInput;
                vertexInput.location = input.location; // Shader 中的 layout(location = N) 值
                vertexInput.name = input.name;        // Shader 变量名（如 "inPosition"）
                
                // 将 shader 类型映射为 RHI Format
                // input.basetype: SPIRType basetype (3=Int, 4=UInt, 5=Float)
                // input.width: 类型宽度 (8, 16, 32, 64 bits)
                // input.vecsize: 向量大小 (1=scalar, 2=vec2, 3=vec3, 4=vec4)
                // 例如：vec3 float32 -> basetype=5, width=32, vecsize=3
                vertexInput.format = MapTypeToFormat(input.basetype, input.width, input.vecsize);
                
                vertexInput.offset = 0;    // 将在 BuildVertexInputsFromShader 中根据 format 尺寸计算
                vertexInput.binding = 0;   // 默认使用 binding 0 的顶点缓冲区
                m_VertexInputs.push_back(vertexInput);
            }

            // ----------------------------------------------------------------------------
            // 2. 解析 Push Constants (推送常量)
            // ----------------------------------------------------------------------------
            // 对应 Shader 语法：
            //   layout(push_constant) uniform PushConstants {
            //       mat4 mvpMatrix;
            //       vec4 color;
            //   } pushConstants;
            //
            // 参数说明：
            //   - push_constant_size: Push constant 数据的总字节数
            //     包含所有 push constant 成员的大小（考虑对齐）
            //
            // 格式和尺寸的用处：
            //   - Push constant 是快速更新的小数据块（通常 < 128 字节）
            //   - 不需要 descriptor set，直接在 command buffer 中更新
            //   - 用于频繁变化的数据（如每帧的 MVP 矩阵）
            // ----------------------------------------------------------------------------
            m_PushConstantData.resize(reflection.push_constant_size);
            std::memset(m_PushConstantData.data(), 0, m_PushConstantData.size());

            // ----------------------------------------------------------------------------
            // 3. 解析 Uniform Buffers (统一缓冲区)
            // ----------------------------------------------------------------------------
            // 对应 Shader 语法：
            //   layout(set = 0, binding = 0) uniform UniformBufferObject {
            //       mat4 model;
            //       mat4 view;
            //       mat4 proj;
            //   } ubo;
            //
            // 参数说明：
            //   - set: Descriptor set 索引，对应 shader 中的 layout(set = N)
            //     用于组织相关的资源（如 set 0 用于 per-object，set 1 用于 per-frame）
            //   - binding: 在 descriptor set 中的绑定索引，对应 shader 中的 layout(binding = N)
            //     用于在同一 set 中区分不同的资源
            //   - name: Uniform buffer 在 shader 中的名称（如 "UniformBufferObject"）
            //   - size: Uniform buffer 的总字节数（包含所有成员，考虑对齐）
            //     用于创建 GPU 缓冲区并分配内存
            //
            // 格式和尺寸的用处：
            //   - size 决定了需要创建的 GPU buffer 大小
            //   - set 和 binding 用于创建 descriptor set layout
            //   - data 存储 CPU 端的数据，用于更新 GPU buffer
            //   - 对齐规则：在 GLSL/Vulkan 中，uniform buffer 成员需要按 16 字节对齐
            //     例如：vec3 占用 12 字节，但需要对齐到 16 字节
            // ----------------------------------------------------------------------------
            m_UniformBuffers.clear();
            for (const auto& ub : reflection.uniform_buffers) {
                UniformBufferBinding binding;
                binding.set = ub.set;      // Shader 中的 layout(set = N)
                binding.binding = ub.binding; // Shader 中的 layout(binding = N)
                binding.name = ub.name;    // Shader 中的 uniform buffer 名称
                binding.size = ub.size;    // Uniform buffer 总字节数（考虑对齐）
                
                // 分配 CPU 端数据缓冲区，用于存储和更新 uniform buffer 数据
                binding.data.resize(ub.size);
                std::memset(binding.data.data(), 0, binding.data.size());
                m_UniformBuffers.push_back(std::move(binding));
            }

            // ----------------------------------------------------------------------------
            // 4. 解析 Texture Bindings (纹理绑定：Samplers 和 Images)
            // ----------------------------------------------------------------------------
            // 对应 Shader 语法：
            //   // Combined Image Sampler (GLSL)
            //   layout(set = 0, binding = 1) uniform sampler2D texSampler;
            //   
            //   // Separate Image and Sampler (HLSL/Vulkan)
            //   layout(set = 0, binding = 1) uniform texture2D texImage;
            //   layout(set = 0, binding = 2) uniform sampler texSampler;
            //
            // 参数说明：
            //   - samplers: Combined image sampler 或单独的 sampler
            //     对应 GLSL 的 sampler2D, samplerCube 等
            //   - images: 单独的 texture image（不包含 sampler）
            //     对应 HLSL 的 texture2D, textureCube 等
            //   - set: Descriptor set 索引，对应 shader 中的 layout(set = N)
            //   - binding: 在 descriptor set 中的绑定索引，对应 shader 中的 layout(binding = N)
            //   - name: 纹理在 shader 中的变量名（如 "texSampler", "albedoMap"）
            //
            // 格式和尺寸的用处：
            //   - set 和 binding 用于创建 descriptor set layout
            //   - 用于在渲染时绑定正确的纹理到 shader
            //   - texture 指针在运行时设置（通过 SetTexture 方法）
            //   - 支持在同一个 descriptor set 中绑定多个纹理（通过不同的 binding）
            // ----------------------------------------------------------------------------
            m_TextureBindings.clear();
            
            // 解析 Samplers（Combined Image Sampler 或单独的 Sampler）
            for (const auto& sampler : reflection.samplers) {
                TextureBinding binding;
                binding.set = sampler.set;      // Shader 中的 layout(set = N)
                binding.binding = sampler.binding; // Shader 中的 layout(binding = N)
                binding.name = sampler.name;     // Shader 中的 sampler 变量名
                binding.texture = nullptr;       // 纹理指针，在运行时通过 SetTexture 设置
                m_TextureBindings.push_back(binding);
            }
            
            // 解析 Images（单独的 Texture Image，不包含 Sampler）
            for (const auto& image : reflection.images) {
                TextureBinding binding;
                binding.set = image.set;         // Shader 中的 layout(set = N)
                binding.binding = image.binding; // Shader 中的 layout(binding = N)
                binding.name = image.name;       // Shader 中的 image 变量名
                binding.texture = nullptr;       // 纹理指针，在运行时通过 SetTexture 设置
                m_TextureBindings.push_back(binding);
            }
        }

        void ShadingMaterial::BuildVertexInputsFromShader() {
            // ============================================================================
            // 计算顶点输入属性的偏移量（Offset）
            // ============================================================================
            // 
            // 用途：
            //   在顶点缓冲区中，多个属性（位置、法线、纹理坐标等）通常打包在同一个缓冲区中。
            //   每个属性需要知道它在缓冲区中的起始位置（offset），以便正确读取数据。
            //
            // 示例顶点数据布局：
            //   struct Vertex {
            //       vec3 position;   // offset = 0,   size = 12 bytes (3 * float)
            //       vec3 normal;     // offset = 12,  size = 12 bytes (3 * float)
            //       vec2 texCoord;   // offset = 24,  size = 8 bytes  (2 * float)
            //   };
            //   Total stride = 32 bytes
            //
            // 计算过程：
            //   1. 第一个属性的 offset = 0
            //   2. 后续属性的 offset = 前一个属性的 offset + 前一个属性的 format 尺寸
            //   3. 最后一个属性的 offset + format 尺寸 = 顶点数据的 stride（步长）
            //
            // Format 尺寸的用处：
            //   - 确定每个属性在内存中占用的字节数
            //   - 用于计算属性之间的偏移量
            //   - 用于计算顶点数据的总 stride
            //   - 确保 GPU 能正确从顶点缓冲区中读取每个属性的数据
            //
            // 注意：
            //   - Format 尺寸必须与实际顶点数据布局匹配
            //   - 如果 shader 中声明的是 vec3，但实际数据是 vec4，需要调整 format
            //   - 对齐规则：某些格式可能需要特定的对齐（如 16 字节对齐）
            // ============================================================================
            uint32_t currentOffset = 0;
            for (auto& input : m_VertexInputs) {
                // 设置当前属性的偏移量
                input.offset = currentOffset;
                
                // 计算当前属性的格式尺寸（字节数）
                // 例如：R8G8B8A8_UNORM = 4 字节，D32_SFLOAT = 4 字节
                uint32_t formatSize = CalculateFormatSize(input.format);
                
                // 更新下一个属性的偏移量
                currentOffset += formatSize;
            }
            
            // 此时 currentOffset 就是顶点数据的 stride（步长）
            // 用于创建 VkVertexInputBindingDescription 中的 stride 字段
        }

        bool ShadingMaterial::DoCreate(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            // Store device reference for cleanup
            m_Device = device;

            if (!m_ShaderCollection) {
                return false;
            }

            // Create shader modules using ShaderModuleTools (separates Device from ShaderCollection)
            m_ShadingState.shaderModules.clear();
            
            auto* collection = static_cast<ShaderCollection*>(m_ShaderCollection);
            auto& moduleTools = ShaderModuleTools::GetInstance();
            
            // Initialize ShaderModuleTools with device if not already initialized
            if (!moduleTools.IsInitialized()) {
                moduleTools.Initialize(device);
            }

            // Get available stages from collection
            auto stages = collection->GetAvailableStages();
            for (ShaderStage stage : stages) {
                // Get SPIR-V code and MD5 hash from collection
                const std::vector<uint32_t>* spirvCode = collection->GetSPIRVCode(stage);
                if (!spirvCode || spirvCode->empty()) {
                    continue;
                }
                
                // Get MD5 hash for this stage
                const std::string& md5Hash = collection->GetMD5Hash(stage);
                if (md5Hash.empty()) {
                    continue;
                }
                
                // Get or create shader module from ShaderModuleTools using shaderID + MD5
                RHI::IShaderModule* shaderModule = moduleTools.GetOrCreateShaderModule(
                    m_ShaderCollectionID,
                    md5Hash,
                    *spirvCode,
                    stage
                );
                
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

            // Create and initialize descriptor manager
            m_DescriptorManager = std::make_unique<MaterialDescriptorManager>();
            if (!m_DescriptorManager->Initialize(this, device)) {
                std::cerr << "Failed to initialize MaterialDescriptorManager!" << std::endl;
                m_DescriptorManager.reset();
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

            // Collect descriptor set layouts from MaterialDescriptorManager
            std::vector<RHI::DescriptorSetLayoutHandle> descriptorSetLayouts;
            if (m_DescriptorManager) {
                auto layouts = m_DescriptorManager->GetAllDescriptorSetLayouts();
                descriptorSetLayouts = layouts;
            }

            // Create pipeline from shading state (with descriptor set layouts)
            if (!m_ShadingState.CreatePipeline(device, renderPass, vertexBindings, vertexAttributes, descriptorSetLayouts)) {
                return false;
            }

            return true;
        }

        bool ShadingMaterial::DoUpdate(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            // Update uniform buffers if data changed
            // This is called from FlushParametersToGPU to transfer CPU data to GPU buffers
            // Note: Per-frame updates should use UpdateUniformBuffer() which handles this automatically
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

            // Cleanup descriptor manager (handles all descriptor-related resources)
            if (m_DescriptorManager && m_Device) {
                m_DescriptorManager->Cleanup(m_Device);
            }
            m_DescriptorManager.reset();

            m_Device = nullptr;
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
            if (!m_DescriptorManager) {
                return nullptr;
            }
            return m_DescriptorManager->GetDescriptorSet(set);
        }

        void* ShadingMaterial::GetDescriptorSetLayout(uint32_t set) const {
            if (!m_DescriptorManager) {
                return nullptr;
            }
            return m_DescriptorManager->GetDescriptorSetLayout(set);
        }

        std::vector<void*> ShadingMaterial::GetAllDescriptorSetLayouts() const {
            if (!m_DescriptorManager) {
                return {};
            }
            auto layouts = m_DescriptorManager->GetAllDescriptorSetLayouts();
            std::vector<void*> result;
            result.reserve(layouts.size());
            for (auto layout : layouts) {
                result.push_back(layout);
            }
            return result;
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


        // ============================================================================
        // Per-frame update interfaces implementation
        // ============================================================================

        bool ShadingMaterial::UpdateTexture(uint32_t set, uint32_t binding, RHI::IImage* texture) {
            TextureBinding* bindingPtr = GetTextureBinding(set, binding);
            if (bindingPtr) {
                bindingPtr->texture = texture;
                return true;
            }
            return false;
        }

        bool ShadingMaterial::UpdateTextureByName(const std::string& name, RHI::IImage* texture) {
            for (auto& tb : m_TextureBindings) {
                if (tb.name == name) {
                    tb.texture = texture;
                    // Update descriptor set bindings through MaterialDescriptorManager
                    if (m_DescriptorManager && m_Device) {
                        m_DescriptorManager->UpdateBindings(this, m_Device);
                    }
                    return true;
                }
            }
            return false;
        }

        bool ShadingMaterial::UpdateUniformBuffer(const UniformBufferUpdate& update) {
            if (!update.data || update.size == 0) {
                return false;
            }

            UniformBufferBinding* ub = GetUniformBuffer(update.set, update.binding);
            if (!ub || !ub->buffer) {
                return false;
            }

            // Validate offset and size
            if (update.offset + update.size > ub->size) {
                return false;
            }

            // Update CPU data
            if (update.offset + update.size > ub->data.size()) {
                ub->data.resize(update.offset + update.size);
            }
            std::memcpy(ub->data.data() + update.offset, update.data, update.size);

            // Update GPU buffer
            ub->buffer->UpdateData(update.data, update.size, update.offset);
            return true;
        }

        bool ShadingMaterial::UpdateUniformBufferByName(const std::string& name, const void* data, uint32_t size, uint32_t offset) {
            if (!data || size == 0) {
                return false;
            }

            for (auto& ub : m_UniformBuffers) {
                if (ub.name == name) {
                    UniformBufferUpdate update;
                    update.set = ub.set;
                    update.binding = ub.binding;
                    update.name = name;
                    update.data = data;
                    update.size = size;
                    update.offset = offset;
                    return UpdateUniformBuffer(update);
                }
            }
            return false;
        }

        bool ShadingMaterial::UpdatePushConstant(const PushConstantUpdate& update) {
            if (!update.data || update.size == 0) {
                return false;
            }

            // Validate offset and size
            if (update.offset + update.size > m_PushConstantData.size()) {
                return false;
            }

            // Update push constant data
            std::memcpy(m_PushConstantData.data() + update.offset, update.data, update.size);
            return true;
        }

        // ============================================================================
        // Geometry validation interface implementation
        // ============================================================================

        bool ShadingMaterial::ValidateVertexInputs(const RenderGeometry* geometry) const {
            if (!geometry) {
                return false;
            }

            // Get vertex stride from geometry
            uint32_t geometryStride = geometry->GetVertexStride();
            if (geometryStride == 0) {
                return false;
            }

            // Calculate expected stride from vertex inputs
            uint32_t expectedStride = 0;
            if (!m_VertexInputs.empty()) {
                const ShadingMaterial::VertexInput& lastInput = m_VertexInputs.back();
                // Calculate format size (simplified - assumes standard formats)
                uint32_t formatSize = CalculateFormatSize(lastInput.format);
                expectedStride = lastInput.offset + formatSize;
            }

            // Check if strides match
            if (expectedStride != geometryStride) {
                return false;
            }

            // Additional validation: check if all required vertex inputs can be satisfied
            // This is a simplified check - in a full implementation, you might want to
            // verify that the geometry data actually contains the required attributes
            return true;
        }

        // ============================================================================
        // Render parameter management implementation
        // ============================================================================

        bool ShadingMaterial::SetRenderParameter(const std::string& key, const RenderParameterValue& value) {
            m_RenderParameters[key] = value;
            return true;
        }

        const ShadingMaterial::RenderParameterValue* ShadingMaterial::GetRenderParameter(const std::string& key) const {
            auto it = m_RenderParameters.find(key);
            if (it != m_RenderParameters.end()) {
                return &it->second;
            }
            return nullptr;
        }

        bool ShadingMaterial::UpdateRenderParameters() {
            // Apply all pending render parameters to CPU-side data (including textures)
            for (const auto& [key, value] : m_RenderParameters) {
                ApplyRenderParameter(key, value, true); // Include textures
            }
            return true;
        }

        void ShadingMaterial::ApplyParameters(const RenderParameterCollector& collector) {
            // Apply all parameters from collector to this material
            const auto& params = collector.GetParameters();
            for (const auto& [key, value] : params) {
                // Convert Renderer::RenderParameterValue to ShadingMaterial::RenderParameterValue
                RenderParameterValue materialValue;
                using ParamType = RenderParameterValue::Type;
                switch (value.type) {
                    case ParamType::Texture:
                        materialValue = RenderParameterValue(value.GetTexture());
                        break;
                    case ParamType::Float:
                        materialValue = RenderParameterValue(value.GetFloat());
                        break;
                    case ParamType::Vec2:
                        materialValue = RenderParameterValue(value.GetVec2());
                        break;
                    case ParamType::Vec3:
                        materialValue = RenderParameterValue(value.GetVec3());
                        break;
                    case ParamType::Vec4:
                        materialValue = RenderParameterValue(value.GetVec4());
                        break;
                    case ParamType::Int:
                        materialValue = RenderParameterValue(value.GetInt());
                        break;
                    case ParamType::Bool:
                        materialValue = RenderParameterValue(value.GetBool());
                        break;
                    case ParamType::Mat3:
                        materialValue = RenderParameterValue(value.GetMat3());
                        break;
                    case ParamType::Mat4:
                        materialValue = RenderParameterValue(value.GetMat4());
                        break;
                    case ParamType::RawData:
                        materialValue = RenderParameterValue(value.GetRawData(), value.GetRawDataSize());
                        break;
                    case ParamType::PushConstant:
                        materialValue = RenderParameterValue(value.GetRawData(), value.GetRawDataSize(), value.GetOffset());
                        break;
                }
                SetRenderParameter(key, materialValue);
            }
        }

        bool ShadingMaterial::ApplyRenderParameter(const std::string& key, const RenderParameterValue& value, bool includeTextures) {
            switch (value.type) {
                case RenderParameterValue::Type::Texture: {
                    if (includeTextures) {
                        // Update texture by name
                        RHI::IImage* texture = value.GetTexture();
                        return UpdateTextureByName(key, texture);
                    }
                    // Textures don't need to be flushed to GPU buffers
                    return true;
                }
                case RenderParameterValue::Type::Float:
                case RenderParameterValue::Type::Vec2:
                case RenderParameterValue::Type::Vec3:
                case RenderParameterValue::Type::Vec4:
                case RenderParameterValue::Type::Int:
                case RenderParameterValue::Type::Bool:
                case RenderParameterValue::Type::Mat3:
                case RenderParameterValue::Type::Mat4:
                case RenderParameterValue::Type::RawData: {
                    // Update uniform buffer by name (updates CPU-side data)
                    const void* data = value.GetRawData();
                    uint32_t size = value.GetRawDataSize();
                    if (data && size > 0) {
                        return UpdateUniformBufferByName(key, data, size, 0);
                    }
                    return false;
                }
                case RenderParameterValue::Type::PushConstant: {
                    // Update push constant (CPU-side data)
                    const void* data = value.GetRawData();
                    uint32_t size = value.GetRawDataSize();
                    uint32_t offset = value.GetOffset();
                    if (data && size > 0) {
                        PushConstantUpdate update;
                        update.data = data;
                        update.size = size;
                        update.offset = offset;
                        return UpdatePushConstant(update);
                    }
                    return false;
                }
                default:
                    return false;
            }
        }

        bool ShadingMaterial::FlushParametersToGPU(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            // Apply parameters to CPU-side data (excluding textures, as they don't need flushing)
            // Textures are already bound and don't need to be flushed to GPU buffers
            for (const auto& [key, value] : m_RenderParameters) {
                ApplyRenderParameter(key, value, false); // Exclude textures
            }

            // Flush CPU-side data to GPU buffers
            // This transfers uniform buffer data from CPU to GPU
            if (!DoUpdate(device)) {
                return false;
            }

            // Update descriptor set bindings through MaterialDescriptorManager
            // This ensures descriptor sets reflect the latest uniform buffer and texture bindings
            if (m_DescriptorManager) {
                m_DescriptorManager->UpdateBindings(this, device);
            }

            return true;
        }

        // ============================================================================
        // RenderParameterValue implementation
        // ============================================================================

        ShadingMaterial::RenderParameterValue::RenderParameterValue(RHI::IImage* texture) : type(Type::Texture), m_Offset(0) {
            data.resize(sizeof(RHI::IImage*));
            std::memcpy(data.data(), &texture, sizeof(RHI::IImage*));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(float value) : type(Type::Float), m_Offset(0) {
            data.resize(sizeof(float));
            std::memcpy(data.data(), &value, sizeof(float));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const Core::Vec2& value) : type(Type::Vec2), m_Offset(0) {
            data.resize(sizeof(Core::Vec2));
            std::memcpy(data.data(), &value, sizeof(Core::Vec2));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const Core::Vec3& value) : type(Type::Vec3), m_Offset(0) {
            data.resize(sizeof(Core::Vec3));
            std::memcpy(data.data(), &value, sizeof(Core::Vec3));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const Core::Vec4& value) : type(Type::Vec4), m_Offset(0) {
            data.resize(sizeof(Core::Vec4));
            std::memcpy(data.data(), &value, sizeof(Core::Vec4));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(int32_t value) : type(Type::Int), m_Offset(0) {
            data.resize(sizeof(int32_t));
            std::memcpy(data.data(), &value, sizeof(int32_t));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(bool value) : type(Type::Bool), m_Offset(0) {
            data.resize(sizeof(bool));
            std::memcpy(data.data(), &value, sizeof(bool));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const Core::Mat3& value) : type(Type::Mat3), m_Offset(0) {
            data.resize(sizeof(Core::Mat3));
            std::memcpy(data.data(), &value, sizeof(Core::Mat3));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const Core::Mat4& value) : type(Type::Mat4), m_Offset(0) {
            data.resize(sizeof(Core::Mat4));
            std::memcpy(data.data(), &value, sizeof(Core::Mat4));
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const void* rawData, uint32_t size) : type(Type::RawData), m_Offset(0) {
            data.resize(size);
            std::memcpy(data.data(), rawData, size);
        }

        ShadingMaterial::RenderParameterValue::RenderParameterValue(const void* rawData, uint32_t size, uint32_t offset) : type(Type::PushConstant), m_Offset(offset) {
            data.resize(size);
            std::memcpy(data.data(), rawData, size);
        }

        RHI::IImage* ShadingMaterial::RenderParameterValue::GetTexture() const {
            if (type == Type::Texture && data.size() >= sizeof(RHI::IImage*)) {
                RHI::IImage* texture = nullptr;
                std::memcpy(&texture, data.data(), sizeof(RHI::IImage*));
                return texture;
            }
            return nullptr;
        }

        float ShadingMaterial::RenderParameterValue::GetFloat() const {
            if (type == Type::Float && data.size() >= sizeof(float)) {
                float value = 0.0f;
                std::memcpy(&value, data.data(), sizeof(float));
                return value;
            }
            return 0.0f;
        }

        Core::Vec2 ShadingMaterial::RenderParameterValue::GetVec2() const {
            if (type == Type::Vec2 && data.size() >= sizeof(Core::Vec2)) {
                Core::Vec2 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec2));
                return value;
            }
            return Core::Vec2(0.0f);
        }

        Core::Vec3 ShadingMaterial::RenderParameterValue::GetVec3() const {
            if (type == Type::Vec3 && data.size() >= sizeof(Core::Vec3)) {
                Core::Vec3 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec3));
                return value;
            }
            return Core::Vec3(0.0f);
        }

        Core::Vec4 ShadingMaterial::RenderParameterValue::GetVec4() const {
            if (type == Type::Vec4 && data.size() >= sizeof(Core::Vec4)) {
                Core::Vec4 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec4));
                return value;
            }
            return Core::Vec4(0.0f);
        }

        int32_t ShadingMaterial::RenderParameterValue::GetInt() const {
            if (type == Type::Int && data.size() >= sizeof(int32_t)) {
                int32_t value = 0;
                std::memcpy(&value, data.data(), sizeof(int32_t));
                return value;
            }
            return 0;
        }

        bool ShadingMaterial::RenderParameterValue::GetBool() const {
            if (type == Type::Bool && data.size() >= sizeof(bool)) {
                bool value = false;
                std::memcpy(&value, data.data(), sizeof(bool));
                return value;
            }
            return false;
        }

        Core::Mat3 ShadingMaterial::RenderParameterValue::GetMat3() const {
            if (type == Type::Mat3 && data.size() >= sizeof(Core::Mat3)) {
                Core::Mat3 value;
                std::memcpy(&value, data.data(), sizeof(Core::Mat3));
                return value;
            }
            return Core::Mat3(1.0f);
        }

        Core::Mat4 ShadingMaterial::RenderParameterValue::GetMat4() const {
            if (type == Type::Mat4 && data.size() >= sizeof(Core::Mat4)) {
                Core::Mat4 value;
                std::memcpy(&value, data.data(), sizeof(Core::Mat4));
                return value;
            }
            return Core::Mat4Identity();
        }

    } // namespace Renderer
} // namespace FirstEngine
