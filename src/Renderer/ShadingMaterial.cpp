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
                        // Note: paramValue is MaterialParameterValue, which has GetData() and GetDataSize()
                        const void* data = paramValue.GetData();
                        uint32_t size = paramValue.GetDataSize();
                        if (data && size > 0 && size <= ub->size) {
                            std::memcpy(ub->data.data(), data, std::min(size, ub->size));
                            parameterSet = true;
                            break;
                        }
                    }
                    
                    // Try to match parameter to uniform buffer members
                    // Use actual member offsets from shader reflection if available
                    for (const auto& member : ubReflection.members) {
                        // Check if parameter name matches member name
                        if (member.name == paramName) {
                            // Get actual member offset from reflection
                            uint32_t memberOffset = member.offset;
                            
                            // If offset is 0 (not set by compiler), calculate it from previous members
                            // This handles std140 layout alignment
                            if (memberOffset == 0 && !ubReflection.members.empty()) {
                                uint32_t calculatedOffset = 0;
                                for (const auto& prevMember : ubReflection.members) {
                                    if (prevMember.name == member.name) {
                                        break; // Found the target member
                                    }
                                    // Add size of previous member (with std140 alignment)
                                    // std140: scalars/vectors aligned to 16 bytes, matrices aligned to 16 bytes
                                    uint32_t prevSize = prevMember.size > 0 ? prevMember.size : 64; // Default to 64 for mat4
                                    // Align to 16-byte boundary (std140 layout)
                                    calculatedOffset = (calculatedOffset + 15) & ~15;
                                    calculatedOffset += prevSize;
                                }
                                // Align final offset to 16-byte boundary
                                memberOffset = (calculatedOffset + 15) & ~15;
                            }
                            
                            // Note: paramValue is MaterialParameterValue, which has GetData() and GetDataSize()
                            const void* data = paramValue.GetData();
                            uint32_t size = paramValue.GetDataSize();
                            
                            // Ensure we don't overflow the buffer
                            if (memberOffset + size <= ub->size) {
                                std::memcpy(ub->data.data() + memberOffset, data, size);
                                parameterSet = true;
                                break;
                            } else {
                                std::cerr << "Warning: ShadingMaterial::ApplyRenderParameter: Parameter '" << paramName 
                                          << "' size " << size << " exceeds buffer size at offset " << memberOffset 
                                          << " (buffer size: " << ub->size << ")" << std::endl;
                            }
                        }
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
                            // Note: paramValue is MaterialParameterValue, which has GetData() and GetDataSize()
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
            // from ShadingMaterial::DoCreate() after GPU resources are created
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
        // Helper function: Map SPIR-V types to RHI Format
        // ============================================================================
        // 
        // Parameter description:
        //   - basetype: SPIRType base type
        //     * 3 = Int (signed integer)
        //     * 4 = UInt (unsigned integer)
        //     * 5 = Float (floating point)
        //   - width: Type width (bits)
        //     * 8 = 8 bits (int8, uint8, float8)
        //     * 16 = 16 bits (int16, uint16, float16/half)
        //     * 32 = 32 bits (int32, uint32, float32)
        //     * 64 = 64 bits (int64, uint64, float64/double)
        //   - vecsize: Vector size
        //     * 1 = scalar (e.g., float, int)
        //     * 2 = vec2 (2-component vector, e.g., vec2, float2)
        //     * 3 = vec3 (3-component vector, e.g., vec3, float3)
        //     * 4 = vec4 (4-component vector, e.g., vec4, float4)
        //
        // Return value:
        //   RHI::Format - Corresponding vertex attribute format
        //
        // Format usage:
        //   - Determines vertex attribute layout and size in memory
        //   - Used to create VkVertexInputAttributeDescription
        //   - Affects how vertex data is read and interpreted
        //   - Example: vec3 float32 -> 12 bytes, vec4 float32 -> 16 bytes
        //
        // Note:
        //   Current implementation uses R8G8B8A8_UNORM as a generic format (4 bytes),
        //   For non-4-byte aligned types like vec3 (12 bytes), may need adjustment
        // ============================================================================
        static RHI::Format MapTypeToFormat(uint32_t basetype, uint32_t width, uint32_t vecsize) {
            // basetype: SPIRType::BaseType enum values
            // width: 8, 16, 32, 64 bits
            // vecsize: 1=scalar, 2=vec2, 3=vec3, 4=vec4
            //
            // SPIRType::BaseType enum:
            //   0=Unknown, 1=Void, 2=Boolean, 3=SByte, 4=UByte, 5=Short, 6=UShort,
            //   7=Int, 8=UInt, 9=Int64, 10=UInt64, 11=AtomicCounter,
            //   12=Half, 13=Float, 14=Double, ...
            
            // ============================================================================
            // Float types (13=Float, 12=Half, 14=Double)
            // ============================================================================
            if (basetype == 13) { // SPIRType::Float (32-bit float)
                if (width == 32) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R32_SFLOAT;
                        case 2: return RHI::Format::R32G32_SFLOAT;
                        case 3: return RHI::Format::R32G32B32_SFLOAT;
                        case 4: return RHI::Format::R32G32B32A32_SFLOAT;
                        default: return RHI::Format::R32_SFLOAT;
                    }
                } else if (width == 16) {
                    // 32-bit float type but 16-bit width (unusual, treat as half)
                    switch (vecsize) {
                        case 1: return RHI::Format::R16_SFLOAT;
                        case 2: return RHI::Format::R16G16_SFLOAT;
                        case 3: return RHI::Format::R16G16B16_SFLOAT;
                        case 4: return RHI::Format::R16G16B16A16_SFLOAT;
                        default: return RHI::Format::R16_SFLOAT;
                    }
                } else if (width == 64) {
                    // 64-bit float (double precision)
                    switch (vecsize) {
                        case 1: return RHI::Format::R64_SFLOAT;
                        case 2: return RHI::Format::R64G64_SFLOAT;
                        case 3: return RHI::Format::R64G64B64_SFLOAT;
                        case 4: return RHI::Format::R64G64B64A64_SFLOAT;
                        default: return RHI::Format::R64_SFLOAT;
                    }
                } else {
                    // Unknown width: Default to 16-bit (2 bytes per component)
                    switch (vecsize) {
                        case 1: return RHI::Format::R16_SFLOAT;
                        case 2: return RHI::Format::R16G16_SFLOAT;
                        case 3: return RHI::Format::R16G16B16_SFLOAT;
                        case 4: return RHI::Format::R16G16B16A16_SFLOAT;
                        default: return RHI::Format::R16_SFLOAT;
                    }
                }
            } else if (basetype == 12) { // SPIRType::Half (16-bit float)
                switch (vecsize) {
                    case 1: return RHI::Format::R16_SFLOAT;
                    case 2: return RHI::Format::R16G16_SFLOAT;
                    case 3: return RHI::Format::R16G16B16_SFLOAT;
                    case 4: return RHI::Format::R16G16B16A16_SFLOAT;
                    default: return RHI::Format::R16_SFLOAT;
                }
            } else if (basetype == 14) { // SPIRType::Double (64-bit float)
                switch (vecsize) {
                    case 1: return RHI::Format::R64_SFLOAT;
                    case 2: return RHI::Format::R64G64_SFLOAT;
                    case 3: return RHI::Format::R64G64B64_SFLOAT;
                    case 4: return RHI::Format::R64G64B64A64_SFLOAT;
                    default: return RHI::Format::R64_SFLOAT;
                }
            }
            // ============================================================================
            // Signed Integer types (7=Int, 5=Short, 3=SByte, 9=Int64)
            // ============================================================================
            else if (basetype == 7) { // SPIRType::Int (32-bit signed integer)
                if (width == 32) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R32_SINT;
                        case 2: return RHI::Format::R32G32_SINT;
                        case 3: return RHI::Format::R32G32B32_SINT;
                        case 4: return RHI::Format::R32G32B32A32_SINT;
                        default: return RHI::Format::R32_SINT;
                    }
                } else if (width == 16) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R16_SINT;
                        case 2: return RHI::Format::R16G16_SINT;
                        case 3: return RHI::Format::R16G16B16_SINT;
                        case 4: return RHI::Format::R16G16B16A16_SINT;
                        default: return RHI::Format::R16_SINT;
                    }
                } else if (width == 8) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R8_SINT;
                        case 2: return RHI::Format::R8G8_SINT;
                        case 3: return RHI::Format::R8G8B8A8_SINT; // vec3 uses vec4 format
                        case 4: return RHI::Format::R8G8B8A8_SINT;
                        default: return RHI::Format::R8_SINT;
                    }
                } else if (width == 64) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R64_SINT;
                        case 2: return RHI::Format::R64G64_SINT;
                        case 3: return RHI::Format::R64G64B64_SINT;
                        case 4: return RHI::Format::R64G64B64A64_SINT;
                        default: return RHI::Format::R64_SINT;
                    }
                } else {
                    // Unknown width: Default to 32-bit
                    switch (vecsize) {
                        case 1: return RHI::Format::R32_SINT;
                        case 2: return RHI::Format::R32G32_SINT;
                        case 3: return RHI::Format::R32G32B32_SINT;
                        case 4: return RHI::Format::R32G32B32A32_SINT;
                        default: return RHI::Format::R32_SINT;
                    }
                }
            } else if (basetype == 5) { // SPIRType::Short (16-bit signed integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R16_SINT;
                    case 2: return RHI::Format::R16G16_SINT;
                    case 3: return RHI::Format::R16G16B16_SINT;
                    case 4: return RHI::Format::R16G16B16A16_SINT;
                    default: return RHI::Format::R16_SINT;
                }
            } else if (basetype == 3) { // SPIRType::SByte (8-bit signed integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R8_SINT;
                    case 2: return RHI::Format::R8G8_SINT;
                    case 3: return RHI::Format::R8G8B8A8_SINT; // vec3 uses vec4 format
                    case 4: return RHI::Format::R8G8B8A8_SINT;
                    default: return RHI::Format::R8_SINT;
                }
            } else if (basetype == 9) { // SPIRType::Int64 (64-bit signed integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R64_SINT;
                    case 2: return RHI::Format::R64G64_SINT;
                    case 3: return RHI::Format::R64G64B64_SINT;
                    case 4: return RHI::Format::R64G64B64A64_SINT;
                    default: return RHI::Format::R64_SINT;
                }
            }
            // ============================================================================
            // Unsigned Integer types (8=UInt, 6=UShort, 4=UByte, 10=UInt64)
            // ============================================================================
            else if (basetype == 8) { // SPIRType::UInt (32-bit unsigned integer)
                if (width == 32) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R32_UINT;
                        case 2: return RHI::Format::R32G32_UINT;
                        case 3: return RHI::Format::R32G32B32_UINT;
                        case 4: return RHI::Format::R32G32B32A32_UINT;
                        default: return RHI::Format::R32_UINT;
                    }
                } else if (width == 16) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R16_UINT;
                        case 2: return RHI::Format::R16G16_UINT;
                        case 3: return RHI::Format::R16G16B16_UINT;
                        case 4: return RHI::Format::R16G16B16A16_UINT;
                        default: return RHI::Format::R16_UINT;
                    }
                } else if (width == 8) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R8_UINT;
                        case 2: return RHI::Format::R8G8_UINT;
                        case 3: return RHI::Format::R8G8B8A8_UINT; // vec3 uses vec4 format
                        case 4: return RHI::Format::R8G8B8A8_UINT;
                        default: return RHI::Format::R8_UINT;
                    }
                } else if (width == 64) {
                    switch (vecsize) {
                        case 1: return RHI::Format::R64_UINT;
                        case 2: return RHI::Format::R64G64_UINT;
                        case 3: return RHI::Format::R64G64B64_UINT;
                        case 4: return RHI::Format::R64G64B64A64_UINT;
                        default: return RHI::Format::R64_UINT;
                    }
                } else {
                    // Unknown width: Default to 32-bit
                    switch (vecsize) {
                        case 1: return RHI::Format::R32_UINT;
                        case 2: return RHI::Format::R32G32_UINT;
                        case 3: return RHI::Format::R32G32B32_UINT;
                        case 4: return RHI::Format::R32G32B32A32_UINT;
                        default: return RHI::Format::R32_UINT;
                    }
                }
            } else if (basetype == 6) { // SPIRType::UShort (16-bit unsigned integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R16_UINT;
                    case 2: return RHI::Format::R16G16_UINT;
                    case 3: return RHI::Format::R16G16B16_UINT;
                    case 4: return RHI::Format::R16G16B16A16_UINT;
                    default: return RHI::Format::R16_UINT;
                }
            } else if (basetype == 4) { // SPIRType::UByte (8-bit unsigned integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R8_UINT;
                    case 2: return RHI::Format::R8G8_UINT;
                    case 3: return RHI::Format::R8G8B8A8_UINT; // vec3 uses vec4 format
                    case 4: return RHI::Format::R8G8B8A8_UINT;
                    default: return RHI::Format::R8_UINT;
                }
            } else if (basetype == 10) { // SPIRType::UInt64 (64-bit unsigned integer)
                switch (vecsize) {
                    case 1: return RHI::Format::R64_UINT;
                    case 2: return RHI::Format::R64G64_UINT;
                    case 3: return RHI::Format::R64G64B64_UINT;
                    case 4: return RHI::Format::R64G64B64A64_UINT;
                    default: return RHI::Format::R64_UINT;
                }
            }
            
            // ============================================================================
            // Default fallback: use 16-bit float (2 bytes per component)
            // This matches typical geometry data and provides good precision/performance balance
            // ============================================================================
            switch (vecsize) {
                case 1: return RHI::Format::R16_SFLOAT;
                case 2: return RHI::Format::R16G16_SFLOAT;
                case 3: return RHI::Format::R16G16B16_SFLOAT;
                case 4: return RHI::Format::R16G16B16A16_SFLOAT;
                default: return RHI::Format::R16_SFLOAT;
            }
        }

        // ============================================================================
        // Helper function: Calculate format byte size
        // ============================================================================
        // 
        // Parameter description:
        //   - format: RHI::Format enum value, represents vertex attribute data format
        //
        // Return value:
        //   uint32_t - Number of bytes occupied by the format
        //
        // Format description:
        //   - R8G8B8A8_UNORM: 4 8-bit unsigned normalized channels (R, G, B, A) = 4 bytes
        //   - R8G8B8A8_SRGB: 4 8-bit sRGB channels (R, G, B, A) = 4 bytes
        //   - B8G8R8A8_UNORM: 4 8-bit unsigned normalized channels (B, G, R, A) = 4 bytes
        //   - B8G8R8A8_SRGB: 4 8-bit sRGB channels (B, G, R, A) = 4 bytes
        //   - D32_SFLOAT: 32-bit depth floating point = 4 bytes
        //   - D24_UNORM_S8_UINT: 24-bit depth + 8-bit stencil = 4 bytes (packed)
        //
        // Size usage:
        //   - Used to calculate vertex attribute offset in buffer
        //   - Used to calculate vertex data stride
        //   - Ensures GPU can correctly read and interpret vertex data
        //   - Affects memory alignment and performance (some formats may require specific alignment)
        //
        // Note:
        //   - Format size must match actual vertex data layout
        //   - Some formats may need to consider alignment requirements (e.g., 16-byte alignment)
        //   - Current implementation returns 4 bytes for all formats (simplified implementation)
        //     Real projects may need to support more formats (e.g., R32G32B32_SFLOAT = 12 bytes)
        // ============================================================================
        static uint32_t CalculateFormatSize(RHI::Format format) {
            switch (format) {
                // 8-bit formats
                case RHI::Format::R8_UNORM:
                case RHI::Format::R8_SNORM:
                case RHI::Format::R8_UINT:
                case RHI::Format::R8_SINT:
                    return 1;
                case RHI::Format::R8G8_UNORM:
                case RHI::Format::R8G8_SNORM:
                case RHI::Format::R8G8_UINT:
                case RHI::Format::R8G8_SINT:
                    return 2;
                case RHI::Format::R8G8B8A8_UNORM:
                case RHI::Format::R8G8B8A8_SNORM:
                case RHI::Format::R8G8B8A8_UINT:
                case RHI::Format::R8G8B8A8_SINT:
                case RHI::Format::R8G8B8A8_SRGB:
                case RHI::Format::B8G8R8A8_UNORM:
                case RHI::Format::B8G8R8A8_SRGB:
                    return 4;
                
                // 16-bit integer formats
                case RHI::Format::R16_UINT:
                case RHI::Format::R16_SINT:
                case RHI::Format::R16_UNORM:
                case RHI::Format::R16_SNORM:
                case RHI::Format::R16_SFLOAT:
                    return 2;
                case RHI::Format::R16G16_UINT:
                case RHI::Format::R16G16_SINT:
                case RHI::Format::R16G16_UNORM:
                case RHI::Format::R16G16_SNORM:
                case RHI::Format::R16G16_SFLOAT:
                    return 4;
                case RHI::Format::R16G16B16_UINT:
                case RHI::Format::R16G16B16_SINT:
                case RHI::Format::R16G16B16_UNORM:
                case RHI::Format::R16G16B16_SNORM:
                case RHI::Format::R16G16B16_SFLOAT:
                    return 6;
                case RHI::Format::R16G16B16A16_UINT:
                case RHI::Format::R16G16B16A16_SINT:
                case RHI::Format::R16G16B16A16_UNORM:
                case RHI::Format::R16G16B16A16_SNORM:
                case RHI::Format::R16G16B16A16_SFLOAT:
                    return 8;
                
                // 32-bit integer formats
                case RHI::Format::R32_UINT:
                case RHI::Format::R32_SINT:
                case RHI::Format::R32_SFLOAT:
                    return 4;
                case RHI::Format::R32G32_UINT:
                case RHI::Format::R32G32_SINT:
                case RHI::Format::R32G32_SFLOAT:
                    return 8;
                case RHI::Format::R32G32B32_UINT:
                case RHI::Format::R32G32B32_SINT:
                case RHI::Format::R32G32B32_SFLOAT:
                    return 12;
                case RHI::Format::R32G32B32A32_UINT:
                case RHI::Format::R32G32B32A32_SINT:
                case RHI::Format::R32G32B32A32_SFLOAT:
                    return 16;
                
                // 64-bit integer formats
                case RHI::Format::R64_UINT:
                case RHI::Format::R64_SINT:
                case RHI::Format::R64_SFLOAT:
                    return 8;
                case RHI::Format::R64G64_UINT:
                case RHI::Format::R64G64_SINT:
                case RHI::Format::R64G64_SFLOAT:
                    return 16;
                case RHI::Format::R64G64B64_UINT:
                case RHI::Format::R64G64B64_SINT:
                case RHI::Format::R64G64B64_SFLOAT:
                    return 24;
                case RHI::Format::R64G64B64A64_UINT:
                case RHI::Format::R64G64B64A64_SINT:
                case RHI::Format::R64G64B64A64_SFLOAT:
                    return 32;
                
                // Depth formats
                case RHI::Format::D32_SFLOAT:               // 32-bit depth = 4 bytes
                    return 4;
                case RHI::Format::D24_UNORM_S8_UINT:        // 24-bit depth + 8-bit stencil = 4 bytes (packed)
                    return 4;
                
                default:
                    return 4; // Default fallback: 4 bytes
            }
        }

        void ShadingMaterial::ParseShaderReflection(const Shader::ShaderReflection& reflection) {
            // ============================================================================
            // Parse Shader Reflection data, convert shader resource declarations to rendering pipeline configuration
            // ============================================================================
            // 
            // Shader Reflection is metadata extracted from compiled SPIR-V code,
            // contains all resources declared in shader: inputs, outputs, uniform buffers, textures, etc.
            // 
            // Corresponding Shader syntax example:
            // - Vertex Input:    layout(location = 0) in vec3 inPosition;
            // - Uniform Buffer:  layout(set = 0, binding = 0) uniform UniformBuffer { ... };
            // - Sampler:         layout(set = 0, binding = 1) uniform sampler2D texSampler;
            // - Image:           layout(set = 0, binding = 2) uniform texture2D texImage;
            // - Push Constant:   layout(push_constant) uniform PushConstants { ... };
            // ============================================================================

            // ----------------------------------------------------------------------------
            // 1. Parse Vertex Inputs (vertex input attributes)
            // ----------------------------------------------------------------------------
            // Corresponding Shader syntax:
            //   layout(location = 0) in vec3 inPosition;
            //   layout(location = 1) in vec2 inTexCoord;
            //   layout(location = 2) in vec3 inNormal;
            //
            // Parameter description:
            //   - location: Vertex attribute location index, corresponds to layout(location = N) in shader
            //     Used to identify different attributes in vertex buffer (position, normal, texture coordinates, etc.)
            //   - name: Attribute variable name in shader (e.g., "inPosition", "inTexCoord")
            //   - format: Vertex attribute data format (e.g., R8G8B8A8_UNORM means 4-byte RGBA)
            //     Format determines how vertex data is read and interpreted from memory
            //   - offset: Byte offset of this attribute in vertex buffer
            //     Used to calculate attribute start position in vertex data (calculated in BuildVertexInputsFromShader)
            //   - binding: Vertex buffer binding index (usually 0, means using first vertex buffer)
            //
            // Format and size usage:
            //   - format determines bytes occupied by each attribute (e.g., R8G8B8A8_UNORM = 4 bytes)
            //   - offset is used to correctly position each attribute's data in vertex buffer
            //   - All attributes' offset + format size = vertex data stride
            //   - This information is used to create VkVertexInputAttributeDescription and VkVertexInputBindingDescription
            // ----------------------------------------------------------------------------
            m_VertexInputs.clear();
            for (const auto& input : reflection.stage_inputs) {
                VertexInput vertexInput;
                vertexInput.location = input.location; // layout(location = N) value in shader
                vertexInput.name = input.name;        // Shader variable name (e.g., "inPosition")
                
                // Map shader type to RHI Format
                // input.basetype: SPIRType basetype (3=Int, 4=UInt, 5=Float)
                // input.width: Type width (8, 16, 32, 64 bits)
                // input.vecsize: Vector size (1=scalar, 2=vec2, 3=vec3, 4=vec4)
                // Example: vec3 float32 -> basetype=5, width=32, vecsize=3
                vertexInput.format = MapTypeToFormat(input.basetype, input.width, input.vecsize);
                
                vertexInput.offset = 0;    // Will be calculated based on format size in BuildVertexInputsFromShader
                vertexInput.binding = 0;   // Default to use binding 0 vertex buffer
                m_VertexInputs.push_back(vertexInput);
            }

            // ----------------------------------------------------------------------------
            // 2. Parse Push Constants
            // ----------------------------------------------------------------------------
            // Corresponding Shader syntax:
            //   layout(push_constant) uniform PushConstants {
            //       mat4 mvpMatrix;
            //       vec4 color;
            //   } pushConstants;
            //
            // Parameter description:
            //   - push_constant_size: Total bytes of push constant data
            //     Contains size of all push constant members (considering alignment)
            //
            // Format and size usage:
            //   - Push constant is a small data block for fast updates (usually < 128 bytes)
            //   - Doesn't need descriptor set, updated directly in command buffer
            //   - Used for frequently changing data (e.g., per-frame MVP matrix)
            // ----------------------------------------------------------------------------
            m_PushConstantData.resize(reflection.push_constant_size);
            std::memset(m_PushConstantData.data(), 0, m_PushConstantData.size());

            // ----------------------------------------------------------------------------
            // 3. Parse Uniform Buffers
            // ----------------------------------------------------------------------------
            // Corresponding Shader syntax:
            //   layout(set = 0, binding = 0) uniform UniformBufferObject {
            //       mat4 model;
            //       mat4 view;
            //       mat4 proj;
            //   } ubo;
            //
            // Parameter description:
            //   - set: Descriptor set index, corresponds to layout(set = N) in shader
            //     Used to organize related resources (e.g., set 0 for per-object, set 1 for per-frame)
            //   - binding: Binding index in descriptor set, corresponds to layout(binding = N) in shader
            //     Used to distinguish different resources in the same set
            //   - name: Uniform buffer name in shader (e.g., "UniformBufferObject")
            //   - size: Total bytes of uniform buffer (contains all members, considering alignment)
            //     Used to create GPU buffer and allocate memory
            //
            // Format and size usage:
            //   - size determines GPU buffer size to create
            //   - set and binding are used to create descriptor set layout
            //   - data stores CPU-side data, used to update GPU buffer
            //   - Alignment rules: In GLSL/Vulkan, uniform buffer members need 16-byte alignment
            //     Example: vec3 occupies 12 bytes, but needs to be aligned to 16 bytes
            // ----------------------------------------------------------------------------
            m_UniformBuffers.clear();
            for (const auto& ub : reflection.uniform_buffers) {
                UniformBufferBinding binding;
                binding.set = ub.set;      // layout(set = N) in shader
                binding.binding = ub.binding; // layout(binding = N) in shader
                binding.name = ub.name;    // Uniform buffer name in shader
                binding.size = ub.size;    // Uniform buffer total bytes (considering alignment)
                
                // Allocate CPU-side data buffer for storing and updating uniform buffer data
                binding.data.resize(ub.size);
                std::memset(binding.data.data(), 0, binding.data.size());
                m_UniformBuffers.push_back(std::move(binding));
                
                // Debug: Print uniform buffer information
            }

            // ----------------------------------------------------------------------------
            // 4. Parse Texture Bindings (Samplers and Images)
            // ----------------------------------------------------------------------------
            // Corresponding Shader syntax:
            //   // Combined Image Sampler (GLSL)
            //   layout(set = 0, binding = 1) uniform sampler2D texSampler;
            //   
            //   // Separate Image and Sampler (HLSL/Vulkan)
            //   layout(set = 0, binding = 1) uniform texture2D texImage;
            //   layout(set = 0, binding = 2) uniform sampler texSampler;
            //
            // Parameter description:
            //   - samplers: Combined image sampler or separate sampler
            //     Corresponds to GLSL's sampler2D, samplerCube, etc.
            //   - images: Separate texture image (doesn't include sampler)
            //     Corresponds to HLSL's texture2D, textureCube, etc.
            //   - set: Descriptor set index, corresponds to layout(set = N) in shader
            //   - binding: Binding index in descriptor set, corresponds to layout(binding = N) in shader
            //   - name: Texture variable name in shader (e.g., "texSampler", "albedoMap")
            //
            // Format and size usage:
            //   - set and binding are used to create descriptor set layout
            //   - Used to bind correct texture to shader during rendering
            //   - texture pointer is set at runtime (via SetTexture method)
            //   - Supports binding multiple textures in same descriptor set (via different bindings)
            // ----------------------------------------------------------------------------
            m_TextureBindings.clear();
            
            // Debug: Print reflection information
            // IMPORTANT: If new fields are empty but legacy fields have data, use legacy fields
            // This handles the case where ShaderCollection was cached before we added the new fields
            bool useLegacyFields = reflection.sampled_images.empty() && 
                                   reflection.separate_images.empty() && 
                                   reflection.separate_samplers.empty() &&
                                   (!reflection.samplers.empty() || !reflection.images.empty());
            
            if (useLegacyFields) {
                
                // Use legacy samplers list (contains both sampled_images and separate_samplers)
                // We'll treat them all as COMBINED_IMAGE_SAMPLER for now
                for (const auto& sampler : reflection.samplers) {
                    TextureBinding binding;
                    binding.set = sampler.set;
                    binding.binding = sampler.binding;
                    binding.name = sampler.name;
                    binding.texture = nullptr;
                    binding.descriptorType = RHI::DescriptorType::CombinedImageSampler;
                    m_TextureBindings.push_back(binding);
                }
                
                // Use legacy images list (contains both separate_images and storage_images)
                // We'll treat them as SAMPLED_IMAGE for now
                for (const auto& image : reflection.images) {
                    TextureBinding binding;
                    binding.set = image.set;
                    binding.binding = image.binding;
                    binding.name = image.name;
                    binding.texture = nullptr;
                    binding.descriptorType = RHI::DescriptorType::SampledImage;
                    m_TextureBindings.push_back(binding);
                }
            } else {
                // Use new fields (preferred)
                // Parse Combined Image Samplers (sampler2D in GLSL)
                // These use COMBINED_IMAGE_SAMPLER descriptor type
                for (const auto& sampler : reflection.sampled_images) {
                TextureBinding binding;
                binding.set = sampler.set;      // layout(set = N) in shader
                binding.binding = sampler.binding; // layout(binding = N) in shader
                binding.name = sampler.name;     // Sampler variable name in shader
                binding.texture = nullptr;       // Texture pointer, set at runtime via SetTexture
                binding.descriptorType = RHI::DescriptorType::CombinedImageSampler;
                m_TextureBindings.push_back(binding);
            }
            
            // Parse Separate Images (texture2D in HLSL/Vulkan, used with separate samplers)
            // These use SAMPLED_IMAGE descriptor type
            for (const auto& image : reflection.separate_images) {
                TextureBinding binding;
                binding.set = image.set;         // layout(set = N) in shader
                binding.binding = image.binding; // layout(binding = N) in shader
                binding.name = image.name;       // Image variable name in shader
                binding.texture = nullptr;       // Texture pointer, set at runtime via SetTexture
                binding.descriptorType = RHI::DescriptorType::SampledImage;
                m_TextureBindings.push_back(binding);
            }
            
            // Parse Separate Samplers (sampler in HLSL/Vulkan, used with separate images)
            // Note: Separate samplers don't have a texture, they're just sampler states
            // For now, we'll create a binding entry but it won't have a texture pointer
            // The descriptor type should be SAMPLER, but our RHI layer might not support it yet
            // So we'll use CombinedImageSampler as a fallback, but this might need to be fixed
            for (const auto& sampler : reflection.separate_samplers) {
                TextureBinding binding;
                binding.set = sampler.set;      // layout(set = N) in shader
                binding.binding = sampler.binding; // layout(binding = N) in shader
                binding.name = sampler.name;     // Sampler variable name in shader
                binding.texture = nullptr;       // Separate samplers don't have textures
                // Use DescriptorType::Sampler for separate samplers (RHI now supports it)
                binding.descriptorType = RHI::DescriptorType::Sampler;
                m_TextureBindings.push_back(binding);
            }
            }
        }

        void ShadingMaterial::BuildVertexInputsFromShader() {
            // ============================================================================
            // Calculate vertex input attribute offsets (Offset)
            // ============================================================================
            // 
            // Purpose:
            //   In vertex buffer, multiple attributes (position, normal, texture coordinates, etc.) are usually packed in the same buffer.
            //   Each attribute needs to know its start position (offset) in the buffer to correctly read data.
            //
            // Example vertex data layout:
            //   struct Vertex {
            //       vec3 position;   // offset = 0,   size = 12 bytes (3 * float)
            //       vec3 normal;     // offset = 12,  size = 12 bytes (3 * float)
            //       vec2 texCoord;   // offset = 24,  size = 8 bytes  (2 * float)
            //   };
            //   Total stride = 32 bytes
            //
            // Calculation process:
            //   1. First attribute's offset = 0
            //   2. Subsequent attributes' offset = previous attribute's offset + previous attribute's format size
            //   3. Last attribute's offset + format size = vertex data stride
            //
            // Format size usage:
            //   - Determines bytes occupied by each attribute in memory
            //   - Used to calculate offsets between attributes
            //   - Used to calculate total vertex data stride
            //   - Ensures GPU can correctly read each attribute's data from vertex buffer
            //
            // Note:
            //   - Format size must match actual vertex data layout
            //   - If shader declares vec3 but actual data is vec4, need to adjust format
            //   - Alignment rules: Some formats may require specific alignment (e.g., 16-byte alignment)
            // ============================================================================
            uint32_t currentOffset = 0;
            for (auto& input : m_VertexInputs) {
                // Set current attribute's offset
                input.offset = currentOffset;
                
                // Calculate current attribute's format size (bytes)
                // Example: R8G8B8A8_UNORM = 4 bytes, D32_SFLOAT = 4 bytes
                uint32_t formatSize = CalculateFormatSize(input.format);
                
                // Update next attribute's offset
                currentOffset += formatSize;
            }
            
            // At this point currentOffset is the vertex data stride
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

            // Set textures from MaterialResource if available
            // This is done after ShadingMaterial is created so GPU resources are available
            if (m_MaterialResource) {
                m_MaterialResource->SetTexturesToShadingMaterial(this);
                
                // IMPORTANT: After setting textures, we need to update descriptor sets
                // Since we just created the ShadingMaterial, the descriptor sets are not in use yet
                // So we can safely force update them with the new texture pointers
                if (m_DescriptorManager) {
                    m_DescriptorManager->ForceUpdateAllBindings(this, device);
                }
            }

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

                // Uniform buffers should use HostVisible memory for frequent CPU updates
                // Use HostVisible | HostCoherent for optimal performance with frequent updates
                RHI::MemoryPropertyFlags memoryProperties = static_cast<RHI::MemoryPropertyFlags>(
                    static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostVisible) |
                    static_cast<uint32_t>(RHI::MemoryPropertyFlags::HostCoherent)
                );

                ub.buffer = device->CreateBuffer(
                    ub.size,
                    RHI::BufferUsageFlags::UniformBuffer,
                    memoryProperties
                );
                if (!ub.buffer) {
                    return false;
                }

                // Upload initial data (now safe to map since we use HostVisible memory)
                if (!ub.data.empty()) {
                    void* mapped = ub.buffer->Map();
                    if (mapped) {
                        std::memcpy(mapped, ub.data.data(), ub.data.size());
                        ub.buffer->Unmap();
                    } else {
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
                    // NOTE: We don't immediately call UpdateBindings here because:
                    // 1. The descriptor set might be in use by a command buffer
                    // 2. UpdateBindings will be called in FlushParametersToGPU() or DoUpdate()
                    // 3. The cache in MaterialDescriptorManager will detect the texture pointer change
                    //    and update the descriptor set when it's safe to do so
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
            // NOTE: If you plan to call FlushParametersToGPU() afterwards, you don't need to call this function
            // FlushParametersToGPU() now handles all parameters (including textures) in a single pass
            // This function is kept for backward compatibility and cases where you only want to update parameters
            // without flushing to GPU (e.g., for testing or when parameters are updated but GPU flush happens later)
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
                    // Update uniform buffer by member name (not buffer name)
                    // This matches the logic in InitializeFromMaterialResource
                    const void* data = value.GetRawData();
                    uint32_t size = value.GetRawDataSize();
                    if (!data || size == 0) {
                        return false;
                    }
                    
                    bool parameterSet = false;
                    const auto& reflection = m_ShaderReflection;
                    
                    // Try to match parameter to uniform buffer members using shader reflection
                    for (const auto& ubReflection : reflection.uniform_buffers) {
                        // Find the corresponding UniformBufferBinding
                        UniformBufferBinding* ub = GetUniformBuffer(ubReflection.set, ubReflection.binding);
                        if (!ub) {
                            continue;
                        }
                        
                        // Check if parameter name matches uniform buffer name
                        if (ubReflection.name == key) {
                            // Parameter name matches buffer name - copy entire buffer data
                            if (size <= ub->size) {
                                std::memcpy(ub->data.data(), data, std::min(size, ub->size));
                                parameterSet = true;
                                
                                break;
                            }
                        }
                        
                        // Try to match parameter to uniform buffer members
                        for (const auto& member : ubReflection.members) {
                            // Check if parameter name matches member name
                            if (member.name == key) {
                                // Get actual member offset from reflection
                                uint32_t memberOffset = member.offset;
                                
                                // If offset is 0 (not set by compiler), calculate it from previous members
                                if (memberOffset == 0 && !ubReflection.members.empty()) {
                                    uint32_t calculatedOffset = 0;
                                    for (const auto& prevMember : ubReflection.members) {
                                        if (prevMember.name == member.name) {
                                            break; // Found the target member
                                        }
                                        // Add size of previous member (with std140 alignment)
                                        uint32_t prevSize = prevMember.size > 0 ? prevMember.size : 64; // Default to 64 for mat4
                                        // Align to 16-byte boundary (std140 layout)
                                        calculatedOffset = (calculatedOffset + 15) & ~15;
                                        calculatedOffset += prevSize;
                                    }
                                    // Align final offset to 16-byte boundary
                                    memberOffset = (calculatedOffset + 15) & ~15;
                                }
                                
                                // Ensure we don't overflow the buffer
                                if (memberOffset + size <= ub->size) {
                                    std::memcpy(ub->data.data() + memberOffset, data, size);
                                    parameterSet = true;
                                    break;
                                } else {
                                    std::cerr << "Warning: ShadingMaterial::ApplyRenderParameter: Parameter '" << key 
                                              << "' size " << size << " exceeds buffer size at offset " << memberOffset 
                                              << " (buffer size: " << ub->size << ")" << std::endl;
                                }
                            }
                        }
                        
                        if (parameterSet) {
                            break;
                        }
                    }
                    
                    // Fallback: If not matched to any uniform buffer member, try UpdateUniformBufferByName
                    // This handles cases where the parameter name matches the buffer name
                    if (!parameterSet) {
                        return UpdateUniformBufferByName(key, data, size, 0);
                    }
                    
                    return parameterSet;
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
            
            // DEBUG: Track how many times FlushParametersToGPU is called per frame
            // This helps identify if the same material is being flushed multiple times
            static std::map<ShadingMaterial*, uint32_t> flushCounts;
            static uint64_t frameCounter = 0;
            flushCounts[this]++;
            
            // Log if called more than once (might indicate optimization opportunity)
            if (flushCounts[this] > 1) {
                // Note: This is expected if multiple entities share the same material
                // Each entity needs to update its per-object uniform buffer data
            }

            // Apply parameters to CPU-side data in a single pass
            // Process textures first (if any), then process uniform buffer data
            // This consolidates the logic from UpdateRenderParameters() to avoid duplicate processing
            // when both functions are called sequentially
            for (const auto& [key, value] : m_RenderParameters) {
                // Process all parameters: textures are updated, uniform buffers are prepared for GPU flush
                ApplyRenderParameter(key, value, true); // Include textures (they need descriptor set updates)
            }

            // Flush CPU-side data to GPU buffers
            // This transfers uniform buffer data from CPU to GPU
            if (!DoUpdate(device)) {
                return false;
            }

            // Update descriptor set bindings through MaterialDescriptorManager
            // This ensures descriptor sets reflect the latest uniform buffer and texture bindings
            // 
            // NOTE: UpdateBindings may cause validation warnings if descriptor sets are in use by pending command buffers.
            // Even with UPDATE_UNUSED_WHILE_PENDING_BIT, descriptors that are actually used by shaders cannot be updated.
            // 
            // IMPORTANT: Since we only update buffer content (via UpdateData) and not buffer pointers,
            // we technically don't need to update descriptor set bindings every frame. However, we do it
            // for texture bindings that may change. For uniform buffers, the descriptor set binding
            // (buffer pointer) typically doesn't change, only the buffer content changes.
            //
            // This is called in EntityToRenderItems (before SubmitRenderQueue), so it should be safe.
            // However, if validation errors persist, consider:
            // 1. Only updating descriptor sets when buffer pointers actually change
            // 2. Using multiple descriptor set instances (one per frame)
            // 3. Or waiting for previous command buffers to complete before updating
            if (m_DescriptorManager) {
                m_DescriptorManager->UpdateBindings(this, device);
            }

            return true;
        }
        
        void ShadingMaterial::BeginFrame() {
            if (m_DescriptorManager) {
                m_DescriptorManager->BeginFrame();
            }
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
