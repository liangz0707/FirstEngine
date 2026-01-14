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

        // Handle types (platform-independent)
        using QueueHandle = void*;
        using SemaphoreHandle = void*;
        using FenceHandle = void*;
        using BufferHandle = void*;
        using ImageHandle = void*;

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

        enum class Format : uint32_t {
            Undefined = 0,
            R8G8B8A8_UNORM = 37,
            R8G8B8A8_SRGB = 43,
            B8G8R8A8_UNORM = 44,
            B8G8R8A8_SRGB = 50,
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
        FE_RHI_API struct DeviceInfo {
            std::string deviceName;
            uint32_t apiVersion;
            uint32_t driverVersion;
            uint64_t deviceMemory;
            uint64_t hostMemory;
        };

        FE_RHI_API struct AttachmentDescription {
            Format format;
            uint32_t samples = 1;
            bool loadOpClear = true;
            bool storeOpStore = true;
            bool stencilLoadOpClear = false;
            bool stencilStoreOpStore = false;
            Format initialLayout = Format::Undefined;
            Format finalLayout = Format::Undefined;
        };

        FE_RHI_API struct RenderPassDescription {
            std::vector<AttachmentDescription> colorAttachments;
            AttachmentDescription depthAttachment;
            bool hasDepthAttachment = false;
        };

        FE_RHI_API struct ImageDescription {
            uint32_t width;
            uint32_t height;
            uint32_t depth = 1;
            uint32_t mipLevels = 1;
            uint32_t arrayLayers = 1;
            Format format;
            ImageUsageFlags usage;
            MemoryPropertyFlags memoryProperties;
        };

        FE_RHI_API struct SwapchainDescription {
            uint32_t width;
            uint32_t height;
            Format preferredFormat = Format::B8G8R8A8_UNORM;
            bool vsync = true;
            uint32_t minImageCount = 2;
        };

        FE_RHI_API struct VertexInputBinding {
            uint32_t binding;
            uint32_t stride;
            bool instanced = false;
        };

        FE_RHI_API struct VertexInputAttribute {
            uint32_t location;
            uint32_t binding;
            Format format;
            uint32_t offset;
        };

        FE_RHI_API struct RasterizationState {
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

        FE_RHI_API struct DepthStencilState {
            bool depthTestEnable = true;
            bool depthWriteEnable = true;
            CompareOp depthCompareOp = CompareOp::Less;
            bool depthBoundsTestEnable = false;
            bool stencilTestEnable = false;
        };

        FE_RHI_API struct ColorBlendAttachment {
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

        FE_RHI_API struct ComputePipelineDescription {
            IShaderModule* computeShader = nullptr;
            std::vector<void*> descriptorSetLayouts;
            std::vector<GraphicsPipelineDescription::PushConstantRange> pushConstantRanges;
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
