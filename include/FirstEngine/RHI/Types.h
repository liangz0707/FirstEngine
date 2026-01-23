#pragma once

#include "FirstEngine/RHI/Export.h"
#include <cstdint>
#include <vector>
#include <string>

namespace FirstEngine {
    namespace RHI {

        // Forward declarations
        class IRenderPass;
        class IImageView;
        class IShaderModule;
        class IBuffer;
        class IImage;

        // Handle types (platform-independent)
        using QueueHandle = void*;
        using SemaphoreHandle = void*;
        using FenceHandle = void*;
        using BufferHandle = void*;
        using ImageHandle = void*;
        using DescriptorSetLayoutHandle = void*;
        using DescriptorSetHandle = void*;
        using DescriptorPoolHandle = void*;

        // Enum types
        enum class ShaderStage : uint32_t {
            Vertex = 0x00000001,
            Fragment = 0x00000010,
            Geometry = 0x00000004,
            Compute = 0x00000020,
            TessellationControl = 0x00000008,
            TessellationEvaluation = 0x00000009,
        };

        enum class BufferUsageFlags : uint32_t {
            None = 0,
            VertexBuffer = 0x00000001,
            IndexBuffer = 0x00000002,
            UniformBuffer = 0x00000010,
            StorageBuffer = 0x00000020,
            TransferSrc = 0x00000040,
            TransferDst = 0x00000080,
        };

        enum class MemoryPropertyFlags : uint32_t {
            None = 0,
            DeviceLocal = 0x00000001,
            HostVisible = 0x00000002,
            HostCoherent = 0x00000004,
            HostCached = 0x00000008,
        };

        enum class ImageUsageFlags : uint32_t {
            None = 0,
            Sampled = 0x00000001,
            Storage = 0x00000002,
            ColorAttachment = 0x00000010,
            DepthStencilAttachment = 0x00000020,
            TransferSrc = 0x00000040,
            TransferDst = 0x00000080,
        };

        // Descriptor types
        enum class DescriptorType : uint32_t {
            UniformBuffer = 0,
            CombinedImageSampler = 1,
            SampledImage = 2,
            StorageImage = 3,
            StorageBuffer = 4,
        };

        enum class Format : uint32_t {
            Undefined = 0,
            
            // 8-bit formats
            R8_UNORM = 9,
            R8_SNORM = 10,
            R8_UINT = 13,
            R8_SINT = 14,
            R8G8_UNORM = 16,
            R8G8_SNORM = 17,
            R8G8_UINT = 20,
            R8G8_SINT = 21,
            R8G8B8A8_UNORM = 37,
            R8G8B8A8_SNORM = 38,
            R8G8B8A8_UINT = 41,
            R8G8B8A8_SINT = 42,
            R8G8B8A8_SRGB = 43,
            B8G8R8A8_UNORM = 44,
            B8G8R8A8_SRGB = 50,
            
            // 16-bit integer formats
            R16_UINT = 62,
            R16_SINT = 63,
            R16_UNORM = 64,
            R16_SNORM = 65,
            R16_SFLOAT = 76,              // 1x 16-bit float = 2 bytes
            R16G16_UINT = 68,
            R16G16_SINT = 69,
            R16G16_UNORM = 70,
            R16G16_SNORM = 71,
            R16G16_SFLOAT = 83,           // 2x 16-bit float = 4 bytes
            R16G16B16_UINT = 88,
            R16G16B16_SINT = 89,
            R16G16B16_UNORM = 84,
            R16G16B16_SNORM = 85,
            R16G16B16_SFLOAT = 90,        // 3x 16-bit float = 6 bytes
            R16G16B16A16_UINT = 95,
            R16G16B16A16_SINT = 96,
            R16G16B16A16_UNORM = 91,
            R16G16B16A16_SNORM = 92,
            R16G16B16A16_SFLOAT = 97,    // 4x 16-bit float = 8 bytes
            
            // 32-bit integer formats
            R32_UINT = 98,
            R32_SINT = 99,
            R32_SFLOAT = 100,             // 1x 32-bit float = 4 bytes (shared with float)
            R32G32_UINT = 101,
            R32G32_SINT = 102,
            R32G32_SFLOAT = 103,          // 2x 32-bit float = 8 bytes
            R32G32B32_UINT = 104,
            R32G32B32_SINT = 105,
            R32G32B32_SFLOAT = 106,       // 3x 32-bit float = 12 bytes
            R32G32B32A32_UINT = 107,
            R32G32B32A32_SINT = 108,
            R32G32B32A32_SFLOAT = 109,    // 4x 32-bit float = 16 bytes
            
            // 64-bit integer formats
            R64_UINT = 110,
            R64_SINT = 111,
            R64G64_UINT = 113,
            R64G64_SINT = 114,
            R64G64B64_UINT = 116,
            R64G64B64_SINT = 117,
            R64G64B64A64_UINT = 119,
            R64G64B64A64_SINT = 120,
            
            // 64-bit float formats (double)
            R64_SFLOAT = 112,             // 1x 64-bit float = 8 bytes
            R64G64_SFLOAT = 115,          // 2x 64-bit float = 16 bytes
            R64G64B64_SFLOAT = 118,      // 3x 64-bit float = 24 bytes
            R64G64B64A64_SFLOAT = 121,    // 4x 64-bit float = 32 bytes
            
            // Depth formats
            D32_SFLOAT = 126,
            D24_UNORM_S8_UINT = 130,
        };

        enum class PrimitiveTopology : uint32_t {
            TriangleList = 0,
            TriangleStrip = 1,
            LineList = 2,
            PointList = 3,
        };

        enum class CullMode : uint32_t {
            None = 0,
            Front = 1,
            Back = 2,
            FrontAndBack = 3,
        };

        enum class CompareOp : uint32_t {
            Never = 0,
            Less = 1,
            Equal = 2,
            LessOrEqual = 3,
            Greater = 4,
            NotEqual = 5,
            GreaterOrEqual = 6,
            Always = 7,
        };

        // Struct definitions
        struct DeviceInfo {
            std::string deviceName;
            uint32_t apiVersion;
            uint32_t driverVersion;
            uint64_t deviceMemory;
            uint64_t hostMemory;
        };

        struct AttachmentDescription {
            Format format;
            uint32_t samples = 1;
            bool loadOpClear = true;
            bool storeOpStore = true;
            bool stencilLoadOpClear = false;
            bool stencilStoreOpStore = false;
            Format initialLayout = Format::Undefined;
            Format finalLayout = Format::Undefined;
        };

        struct RenderPassDescription {
            std::vector<AttachmentDescription> colorAttachments;
            AttachmentDescription depthAttachment;
            bool hasDepthAttachment = false;
        };

        struct ImageDescription {
            uint32_t width;
            uint32_t height;
            uint32_t depth = 1;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            Format format;
            ImageUsageFlags usage;
            MemoryPropertyFlags memoryProperties;
        };

        struct SwapchainDescription {
            uint32_t width;
            uint32_t height;
            Format preferredFormat = Format::B8G8R8A8_UNORM;
            bool vsync = true;
            uint32_t minImageCount = 2;
        };

        struct VertexInputBinding {
            uint32_t binding;
            uint32_t stride;
            bool instanced = false;
        };

        struct VertexInputAttribute {
            uint32_t location;
            uint32_t binding;
            Format format;
            uint32_t offset;
        };

        struct RasterizationState {
            bool depthClampEnable = false;
            bool rasterizerDiscardEnable = false;
            CullMode cullMode = CullMode::Back;
            bool frontFaceCounterClockwise = true;
            bool depthBiasEnable = false;
            float depthBiasConstantFactor = 0.0f;
            float depthBiasClamp = 0.0f;
            float depthBiasSlopeFactor = 0.0f;
            float lineWidth = 1.0f;
        };

        struct DepthStencilState {
            bool depthTestEnable = true;
            bool depthWriteEnable = true;
            CompareOp depthCompareOp = CompareOp::Less;
            bool depthBoundsTestEnable = false;
            bool stencilTestEnable = false;
        };

        struct ColorBlendAttachment {
            bool blendEnable = false;
            uint32_t srcColorBlendFactor = 1; // One
            uint32_t dstColorBlendFactor = 0; // Zero
            uint32_t colorBlendOp = 0; // Add
            uint32_t srcAlphaBlendFactor = 1;
            uint32_t dstAlphaBlendFactor = 0;
            uint32_t alphaBlendOp = 0;
        };

        FE_RHI_API struct GraphicsPipelineDescription {
            IRenderPass* renderPass = nullptr;
            std::vector<IShaderModule*> shaderModules;
            
            // Vertex input
            std::vector<VertexInputBinding> vertexBindings;
            std::vector<VertexInputAttribute> vertexAttributes;
            
            // Primitive topology
            PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;
            
            // Viewport and scissor
            struct Viewport {
                float x = 0.0f;
                float y = 0.0f;
                float width = 0.0f;
                float height = 0.0f;
                float minDepth = 0.0f;
                float maxDepth = 1.0f;
            } viewport;
            
            struct Scissor {
                int32_t x = 0;
                int32_t y = 0;
                uint32_t width = 0;
                uint32_t height = 0;
            } scissor;
            
            // Rasterization
            RasterizationState rasterizationState;
            
            // Depth stencil
            DepthStencilState depthStencilState;
            
            // Color blending
            std::vector<ColorBlendAttachment> colorBlendAttachments;
            
            // Descriptor set layouts (temporarily using void*, will be abstracted later)
            std::vector<void*> descriptorSetLayouts;
            
            // Push constants
            struct PushConstantRange {
                uint32_t offset;
                uint32_t size;
                ShaderStage stageFlags;
            };
            std::vector<PushConstantRange> pushConstantRanges;
        };

        struct ComputePipelineDescription {
            IShaderModule* computeShader = nullptr;
            std::vector<void*> descriptorSetLayouts;
            std::vector<GraphicsPipelineDescription::PushConstantRange> pushConstantRanges;
        };

        // Descriptor binding information
        struct DescriptorBinding {
            uint32_t binding = 0;           // Binding index
            DescriptorType type = DescriptorType::UniformBuffer;
            uint32_t count = 1;             // Array size (1 for non-array)
            ShaderStage stageFlags = ShaderStage::Vertex; // Shader stages that use this binding
        };

        // Descriptor set layout description
        struct DescriptorSetLayoutDescription {
            std::vector<DescriptorBinding> bindings;
        };

        // Descriptor write information (for updating descriptor sets)
        struct DescriptorBufferInfo {
            IBuffer* buffer = nullptr;
            uint64_t offset = 0;
            uint64_t range = 0;  // 0 means entire buffer
        };

        struct DescriptorImageInfo {
            IImage* image = nullptr;
            IImageView* imageView = nullptr;
            void* sampler = nullptr;  // Sampler handle (void* for now)
        };

        struct DescriptorWrite {
            DescriptorSetHandle dstSet = nullptr;
            uint32_t dstBinding = 0;
            uint32_t dstArrayElement = 0;  // For array bindings
            DescriptorType descriptorType = DescriptorType::UniformBuffer;
            std::vector<DescriptorBufferInfo> bufferInfo;  // For uniform/storage buffers
            std::vector<DescriptorImageInfo> imageInfo;     // For images/samplers
        };

        // Bitwise operators
        inline BufferUsageFlags operator|(BufferUsageFlags a, BufferUsageFlags b) {
            return static_cast<BufferUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        inline MemoryPropertyFlags operator|(MemoryPropertyFlags a, MemoryPropertyFlags b) {
            return static_cast<MemoryPropertyFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

        inline ImageUsageFlags operator|(ImageUsageFlags a, ImageUsageFlags b) {
            return static_cast<ImageUsageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
        }

    } // namespace RHI
} // namespace FirstEngine
