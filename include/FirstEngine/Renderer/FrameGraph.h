#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/Types.h"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>

namespace FirstEngine {
    namespace Renderer {

        // Forward declarations
        class FrameGraph;
        class FrameGraphNode;
        class FrameGraphResource;
        class FrameGraphBuilder;

        // FrameGraph resource type
        enum class ResourceType {
            Texture,
            Buffer,
            Attachment,
        };

        // FrameGraph resource description
        FE_RENDERER_API struct ResourceDescription {
            ResourceType type;
            std::string name;
            
            // For textures/attachments
            uint32_t width = 0;
            uint32_t height = 0;
            RHI::Format format = RHI::Format::Undefined;
            bool hasDepth = false;
            
            // For buffers
            uint64_t size = 0;
            RHI::BufferUsageFlags bufferUsage = RHI::BufferUsageFlags::None;
            
            // Resource lifecycle
            uint32_t firstPass = 0;
            uint32_t lastPass = 0;
        };

        // FrameGraph node (render pass)
        class FE_RENDERER_API FrameGraphNode {
        public:
            FrameGraphNode(const std::string& name, uint32_t index);
            ~FrameGraphNode();

            const std::string& GetName() const { return m_Name; }
            uint32_t GetIndex() const { return m_Index; }

            // Resource access
            void AddReadResource(const std::string& resourceName);
            void AddWriteResource(const std::string& resourceName);
            const std::vector<std::string>& GetReadResources() const { return m_ReadResources; }
            const std::vector<std::string>& GetWriteResources() const { return m_WriteResources; }

            // Dependencies
            void AddDependency(uint32_t nodeIndex);
            const std::vector<uint32_t>& GetDependencies() const { return m_Dependencies; }

            // Execution callback
            using ExecuteCallback = std::function<void(RHI::ICommandBuffer*, FrameGraphBuilder&)>;
            void SetExecuteCallback(ExecuteCallback callback) { m_ExecuteCallback = callback; }
            const ExecuteCallback& GetExecuteCallback() const { return m_ExecuteCallback; }

        private:
            std::string m_Name;
            uint32_t m_Index;
            std::vector<std::string> m_ReadResources;
            std::vector<std::string> m_WriteResources;
            std::vector<uint32_t> m_Dependencies;
            ExecuteCallback m_ExecuteCallback;
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
            FrameGraphBuilder(FrameGraph* graph);

            // Read resources
            RHI::IImage* ReadTexture(const std::string& name);
            RHI::IBuffer* ReadBuffer(const std::string& name);

            // Write resources
            RHI::IImage* WriteTexture(const std::string& name);
            RHI::IBuffer* WriteBuffer(const std::string& name);

            // Create temporary resources
            std::string CreateTexture(const std::string& name, const ResourceDescription& desc);
            std::string CreateBuffer(const std::string& name, const ResourceDescription& desc);

        private:
            FrameGraph* m_Graph;
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

            // Add resource
            FrameGraphResource* AddResource(const std::string& name, const ResourceDescription& desc);

            // Compile FrameGraph (analyze dependencies, allocate resources)
            bool Compile();

            // Execute FrameGraph (generate actual render commands)
            void Execute(RHI::ICommandBuffer* commandBuffer);

            // Get resource
            FrameGraphResource* GetResource(const std::string& name);
            const FrameGraphResource* GetResource(const std::string& name) const;

            // Get node
            FrameGraphNode* GetNode(uint32_t index);
            const FrameGraphNode* GetNode(uint32_t index) const;
            uint32_t GetNodeCount() const { return static_cast<uint32_t>(m_Nodes.size()); }

            // Clear (for next frame)
            void Clear();

            // Get device
            RHI::IDevice* GetDevice() const { return m_Device; }

        private:
            // Analyze dependencies
            void AnalyzeDependencies();

            // Allocate resources
            bool AllocateResources();

            // Topological sort
            std::vector<uint32_t> TopologicalSort();

            RHI::IDevice* m_Device;
            std::vector<std::unique_ptr<FrameGraphNode>> m_Nodes;
            std::unordered_map<std::string, std::unique_ptr<FrameGraphResource>> m_Resources;
            std::unordered_map<std::string, uint32_t> m_ResourceNameToIndex;
            std::unordered_map<std::string, uint32_t> m_NodeNameToIndex;
        };

    } // namespace Renderer
} // namespace FirstEngine
