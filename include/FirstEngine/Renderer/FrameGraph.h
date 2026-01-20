#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/RenderPassTypes.h"
#include "FirstEngine/Renderer/ResourceTypes.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cstdint>
#include <climits>

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class Scene;
    }
    namespace Renderer {
        class FrameGraphExecutionPlan;
        class RenderConfig;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // Forward declarations
        class FrameGraph;
        class FrameGraphNode;
        class FrameGraphResource;
        class FrameGraphBuilder;

        // FrameGraph resource description base class
        // Concrete resource types (AttachmentResource, BufferResource) inherit from this
        class FE_RENDERER_API ResourceDescription {
        public:
            ResourceDescription(ResourceType type, const std::string& name);
            virtual ~ResourceDescription() = default;

            ResourceType GetType() const { return m_Type; }
            const std::string& GetName() const { return m_Name; }
            
            // For textures/attachments
            uint32_t GetWidth() const { return m_Width; }
            uint32_t GetHeight() const { return m_Height; }
            RHI::Format GetFormat() const { return m_Format; }
            bool HasDepth() const { return m_HasDepth; }
            
            // For buffers
            uint64_t GetSize() const { return m_Size; }
            RHI::BufferUsageFlags GetBufferUsage() const { return m_BufferUsage; }
            
            // Resource lifecycle
            uint32_t GetFirstPass() const { return m_FirstPass; }
            uint32_t GetLastPass() const { return m_LastPass; }
            void SetFirstPass(uint32_t pass) { m_FirstPass = pass; }
            void SetLastPass(uint32_t pass) { m_LastPass = pass; }

        protected:
            ResourceType m_Type;
            std::string m_Name;
            
            // For textures/attachments
            uint32_t m_Width = 0;
            uint32_t m_Height = 0;
            RHI::Format m_Format = RHI::Format::Undefined;
            bool m_HasDepth = false;
            
            // For buffers
            uint64_t m_Size = 0;
            RHI::BufferUsageFlags m_BufferUsage = RHI::BufferUsageFlags::None;
            
            // Resource lifecycle
            uint32_t m_FirstPass = 0;
            uint32_t m_LastPass = 0;
        };

        // Attachment resource (for G-Buffer, final output, etc.)
        class FE_RENDERER_API AttachmentResource : public ResourceDescription {
        public:
            AttachmentResource(
                const std::string& name,
                uint32_t width,
                uint32_t height,
                RHI::Format format,
                bool hasDepth = false
            );
        };

        // Buffer resource
        class FE_RENDERER_API BufferResource : public ResourceDescription {
        public:
            BufferResource(
                const std::string& name,
                uint64_t size,
                RHI::BufferUsageFlags usage
            );
        };

        // Forward declaration
        class FrameGraph;

        // FrameGraph node (render pass)
        class FE_RENDERER_API FrameGraphNode {
        public:
            FrameGraphNode(const std::string& name, uint32_t index = 0xFFFFFFFF);
            virtual ~FrameGraphNode(); // Virtual destructor for polymorphism (required for dynamic_cast)

            const std::string& GetName() const { return m_Name; }
            uint32_t GetIndex() const { return m_Index; }
            void SetIndex(uint32_t index) { m_Index = index; }

            // Set FrameGraph reference (for automatic resource management)
            void SetFrameGraph(FrameGraph* frameGraph) { m_FrameGraph = frameGraph; }
            FrameGraph* GetFrameGraph() const { return m_FrameGraph; }

            // Resource access
            // If FrameGraph is set, these methods will automatically add and allocate resources
            void AddReadResource(const std::string& resourceName, const ResourceDescription* resourceDesc = nullptr);
            void AddWriteResource(const std::string& resourceName, const ResourceDescription* resourceDesc = nullptr);
            const std::vector<std::string>& GetReadResources() const { return m_ReadResources; }
            const std::vector<std::string>& GetWriteResources() const { return m_WriteResources; }
            
            // Clear resource lists (called at the start of each frame's OnBuild)
            void ClearResources() {
                m_ReadResources.clear();
                m_WriteResources.clear();
                m_Dependencies.clear();
            }

            // Dependencies
            void AddDependency(uint32_t nodeIndex);
            const std::vector<uint32_t>& GetDependencies() const { return m_Dependencies; }

            // Execution callback - now returns RenderCommandList instead of writing to CommandBuffer
            // The second parameter is optional scene render commands (for geometry/forward passes)
            using ExecuteCallback = std::function<RenderCommandList(FrameGraphBuilder&, const RenderCommandList*)>;
            void SetExecuteCallback(ExecuteCallback callback) { m_ExecuteCallback = callback; }
            const ExecuteCallback& GetExecuteCallback() const { return m_ExecuteCallback; }

            // Node type (for execution plan) - using enum
            void SetType(RenderPassType type) { m_Type = type; }
            RenderPassType GetType() const { return m_Type; }
            
            // String conversion helpers (for backward compatibility and serialization)
            void SetType(const std::string& typeStr) { m_Type = StringToRenderPassType(typeStr); }
            std::string GetTypeString() const { return RenderPassTypeToString(m_Type); }

        protected:
            std::string m_Name;
            uint32_t m_Index;
            RenderPassType m_Type; // Node type using enum
            std::vector<std::string> m_ReadResources;
            std::vector<std::string> m_WriteResources;
            std::vector<uint32_t> m_Dependencies;
            ExecuteCallback m_ExecuteCallback;
            FrameGraph* m_FrameGraph = nullptr; // Reference to FrameGraph for automatic resource management
        };

        // FrameGraph resource
        class FE_RENDERER_API FrameGraphResource {
        public:
            FrameGraphResource(const std::string& name, const ResourceDescription& desc);
            ~FrameGraphResource();

            const std::string& GetName() const { return m_Name; }
            const ResourceDescription& GetDescription() const { return m_Description; }

            // Resource handle (set after compilation)
            void SetHandle(void* handle) { m_Handle = handle; }
            void* GetHandle() const { return m_Handle; }

            // Actual resource object (set after compilation)
            void SetRHIResource(RHI::IImage* image) { m_RHIImage = image; }
            void SetRHIResource(RHI::IBuffer* buffer) { m_RHIBuffer = buffer; }
            RHI::IImage* GetRHIImage() const { return m_RHIImage; }
            RHI::IBuffer* GetRHIBuffer() const { return m_RHIBuffer; }

        private:
            std::string m_Name;
            ResourceDescription m_Description;
            void* m_Handle = nullptr;
            RHI::IImage* m_RHIImage = nullptr;
            RHI::IBuffer* m_RHIBuffer = nullptr;
        };

        class FE_RENDERER_API FrameGraphBuilder {
        public:
            FrameGraphBuilder(FrameGraph* graph, RHI::IRenderPass* renderPass = nullptr);

            // Read resources
            RHI::IImage* ReadTexture(const std::string& name);
            RHI::IBuffer* ReadBuffer(const std::string& name);

            // Write resources
            RHI::IImage* WriteTexture(const std::string& name);
            RHI::IBuffer* WriteBuffer(const std::string& name);

            // Create temporary resources
            std::string CreateTexture(const std::string& name, const ResourceDescription& desc);
            std::string CreateBuffer(const std::string& name, const ResourceDescription& desc);

            // Get current render pass (for pipeline creation)
            RHI::IRenderPass* GetRenderPass() const { return m_RenderPass; }

        private:
            FrameGraph* m_Graph;
            RHI::IRenderPass* m_RenderPass = nullptr;
        };

        class FE_RENDERER_API FrameGraph {
        public:
            FrameGraph(RHI::IDevice* device);
            ~FrameGraph();

            // Disable copy constructor and copy assignment operator
            FrameGraph(const FrameGraph&) = delete;
            FrameGraph& operator=(const FrameGraph&) = delete;

            // Add node
            FrameGraphNode* AddNode(const std::string& name);
            
            // Add an existing node (for IRenderPass that inherits from FrameGraphNode)
            // If the node is an IRenderPass, automatically sets execute callback from OnDraw()
            FrameGraphNode* AddNode(FrameGraphNode* node);

            // Add resource
            FrameGraphResource* AddResource(const std::string& name, const ResourceDescription& desc);

            // Allocate a single resource (for per-pass resource allocation)
            // Returns true if allocation succeeded
            bool AllocateResource(const std::string& resourceName);

            // Build execution plan (creates intermediate structure without CommandBuffer/Device dependencies)
            // This should be called before OnRender() to determine what scene resources need to be collected
            bool BuildExecutionPlan(FrameGraphExecutionPlan& plan);

            // Compile FrameGraph (analyze dependencies, allocate resources)
            // Note: This now allocates resources based on the execution plan
            bool Compile(const FrameGraphExecutionPlan& plan);

            // Execute FrameGraph (generate render command list from execution plan, no CommandBuffer dependency)
            // Returns a RenderCommandList that can be recorded to CommandBuffer later
            // scene: Scene to render (passed to Passes with SceneRenderer)
            // renderConfig: Render configuration (camera, resolution, flags) - used by SceneRenderer
            // This method will:
            //   1. For each Pass with SceneRenderer, call Render() to generate SceneRenderCommands
            //   2. Pass SceneRenderCommands to OnDraw() callback

            RenderCommandList Execute(const FrameGraphExecutionPlan& plan, Resources::Scene* scene, const RenderConfig& renderConfig);

            // Get resource
            FrameGraphResource* GetResource(const std::string& name);
            const FrameGraphResource* GetResource(const std::string& name) const;

            // Get node
            FrameGraphNode* GetNode(uint32_t index);
            const FrameGraphNode* GetNode(uint32_t index) const;
            uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_Nodes.size()); }

            // Clear (for next frame)
            // This clears the graph structure but does NOT release resources
            void Clear();

            // Release all allocated resources
            // This should be called before rebuilding the FrameGraph
            void ReleaseResources();

            // Get device
            RHI::IDevice* GetDevice() const { return m_Device; }

        private:
            // Analyze dependencies
            void AnalyzeDependencies();

            // Allocate resources
            bool AllocateResources();

            // Topological sort
            std::vector<uint32_t> TopologicalSort();

            // Type alias for node storage with custom deleter support
            using NodePtr = std::unique_ptr<FrameGraphNode, std::function<void(FrameGraphNode*)>>;
            
            RHI::IDevice* m_Device;
            std::vector<NodePtr> m_Nodes;
            std::unordered_map<std::string, std::unique_ptr<FrameGraphResource>> m_Resources;
            std::unordered_map<std::string, uint32_t> m_ResourceNameToIndex;
            std::unordered_map<std::string, uint32_t> m_NodeNameToIndex;
        };

    } // namespace Renderer
} // namespace FirstEngine
