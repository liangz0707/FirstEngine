#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/FrameGraphResourceWrappers.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/Renderer/SceneRenderer.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/IFramebuffer.h"
#include "FirstEngine/RHI/IBuffer.h"
#include "FirstEngine/RHI/IImage.h"
#include <algorithm>
#include <queue>
#include <stdexcept>
#include <memory>
#include <iostream>

namespace FirstEngine {
    namespace Renderer {

        // FrameGraphNode implementation
        FrameGraphNode::FrameGraphNode(const std::string& name, uint32_t index)
            : m_Name(name), m_Index(index), m_Type(RenderPassType::Unknown), m_FrameGraph(nullptr) {
        }

        FrameGraphNode::~FrameGraphNode() {
            // Resources will be destroyed through IRenderResource lifecycle
            // If they are scheduled for destroy, they will be cleaned up in ProcessResources
        }

        void FrameGraphNode::SetFramebufferResource(std::unique_ptr<FramebufferResource> framebuffer) {
            // If we already have a framebuffer resource, schedule it for destruction
            if (m_FramebufferResource && m_FramebufferResource->IsCreated()) {
                m_FramebufferResource->ScheduleDestroy();
            }
            m_FramebufferResource = std::move(framebuffer);
        }

        RHI::IFramebuffer* FrameGraphNode::GetFramebuffer() const {
            if (m_FramebufferResource) {
                return m_FramebufferResource->GetFramebuffer();
            }
            return nullptr;
        }

        void FrameGraphNode::SetRenderPassResource(std::unique_ptr<RenderPassResource> renderPass) {
            // If we already have a render pass resource, schedule it for destruction
            if (m_RenderPassResource && m_RenderPassResource->IsCreated()) {
                m_RenderPassResource->ScheduleDestroy();
            }
            m_RenderPassResource = std::move(renderPass);
        }

        RHI::IRenderPass* FrameGraphNode::GetRenderPass() const {
            if (m_RenderPassResource) {
                return m_RenderPassResource->GetRenderPass();
            }
            return nullptr;
        }

        void FrameGraphNode::AddReadResource(const std::string& resourceName, const ResourceDescription* resourceDesc) {
            m_ReadResources.push_back(resourceName);

            // If FrameGraph is set and resource description is provided, automatically add and allocate resource
            // This allows Passes to declare resources they need without manually calling AddResource/AllocateResource
            if (m_FrameGraph && resourceDesc) {
                // Check if resource already exists
                auto* existingResource = m_FrameGraph->GetResource(resourceName);
                if (!existingResource) {
                    // Add resource
                    m_FrameGraph->AddResource(resourceName, *resourceDesc);
                    // Allocate resource immediately (per-pass resource allocation)
                    m_FrameGraph->AllocateResource(resourceName);
                }
            }
            // If resourceDesc is nullptr, resource should already exist (added by another Pass)
        }

        void FrameGraphNode::AddWriteResource(const std::string& resourceName, const ResourceDescription* resourceDesc) {
            // Debug: Check for suspicious resource names
            if (resourceName.length() == 1 && m_WriteResources.size() > 100) {
                std::cerr << "Warning: Suspicious write resource name detected: '" << resourceName
                    << "' (single character). Current writeResources size: "
                    << m_WriteResources.size() << std::endl;
            }
            m_WriteResources.push_back(resourceName);

            // If FrameGraph is set and resource description is provided, automatically add and allocate resource
            // This allows Passes to declare resources they need without manually calling AddResource/AllocateResource
            if (m_FrameGraph && resourceDesc) {
                // Check if resource already exists
                auto* existingResource = m_FrameGraph->GetResource(resourceName);
                if (!existingResource) {
                    // Add resource
                    m_FrameGraph->AddResource(resourceName, *resourceDesc);
                    // Allocate resource immediately (per-pass resource allocation)
                    m_FrameGraph->AllocateResource(resourceName);
                }
            }
            // If resourceDesc is nullptr, resource should already exist (added by another Pass)
        }

        void FrameGraphNode::AddDependency(uint32_t nodeIndex) {
            m_Dependencies.push_back(nodeIndex);
        }

        // FrameGraphResource implementation
        FrameGraphResource::FrameGraphResource(const std::string& name, const ResourceDescription& desc)
            : m_Name(name), m_Description(desc) {
        }

        FrameGraphResource::~FrameGraphResource() = default;

        // FrameGraphBuilder implementation
        FrameGraphBuilder::FrameGraphBuilder(FrameGraph* graph, RHI::IRenderPass* renderPass, RHI::IFramebuffer* framebuffer)
            : m_Graph(graph), m_RenderPass(renderPass), m_Framebuffer(framebuffer) {
        }

        RHI::IImage* FrameGraphBuilder::ReadTexture(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource) {
                ResourceType type = resource->GetDescription().GetType();
                // Both Texture and Attachment resources contain images
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    return resource->GetRHIImage();
                }
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::ReadBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Buffer) {
                return resource->GetRHIBuffer();
            }
            return nullptr;
        }

        RHI::IImage* FrameGraphBuilder::WriteTexture(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource) {
                ResourceType type = resource->GetDescription().GetType();
                // Both Texture and Attachment resources contain images
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    return resource->GetRHIImage();
                }
            }
            return nullptr;
        }

        RHI::IBuffer* FrameGraphBuilder::WriteBuffer(const std::string& name) {
            auto* resource = m_Graph->GetResource(name);
            if (resource && resource->GetDescription().GetType() == ResourceType::Buffer) {
                return resource->GetRHIBuffer();
            }
            return nullptr;
        }

        std::string FrameGraphBuilder::CreateTexture(const std::string& name, const ResourceDescription& desc) {
            m_Graph->AddResource(name, desc);
            return name;
        }

        std::string FrameGraphBuilder::CreateBuffer(const std::string& name, const ResourceDescription& desc) {
            m_Graph->AddResource(name, desc);
            return name;
        }

        FrameGraph::FrameGraph(RHI::IDevice* device)
            : m_Device(device) {
        }

        FrameGraph::~FrameGraph() = default;

        FrameGraphNode* FrameGraph::AddNode(const std::string& name) {
            if (m_NodeNameToIndex.find(name) != m_NodeNameToIndex.end()) {
                return nullptr;
            }

            uint32_t index = static_cast<uint32_t>(m_Nodes.size());
            auto node = std::unique_ptr<FrameGraphNode, std::function<void(FrameGraphNode*)>>(
                new FrameGraphNode(name, index),
                [](FrameGraphNode* ptr) { delete ptr; }
            );
            FrameGraphNode* nodePtr = node.get();

            // Set FrameGraph reference and index for automatic resource management
            nodePtr->SetFrameGraph(this);
            nodePtr->SetIndex(index);

            m_Nodes.push_back(std::move(node));
            m_NodeNameToIndex[name] = index;
            return nodePtr;
        }

        // Add an existing node (for IRenderPass that inherits from FrameGraphNode)
        // Note: The node's lifetime is managed by the caller (e.g., IRenderPass in DeferredRenderPipeline)
        // FrameGraph only stores a reference, not ownership
        FrameGraphNode* FrameGraph::AddNode(FrameGraphNode* node) {
            if (!node) {
                return nullptr;
            }

            const std::string& name = node->GetName();
            if (m_NodeNameToIndex.find(name) != m_NodeNameToIndex.end()) {
                return nullptr;
            }

            uint32_t index = static_cast<uint32_t>(m_Nodes.size());

            // Clear node resources before adding (important: nodes are reused across frames)
            // Resources will be re-added in OnBuild
            node->ClearResources();

            // IMPORTANT: If node was previously marked for destruction but is being reused,
            // cancel the destruction schedule to prevent premature resource deletion
            // This happens when the same node is used across frames
            auto* framebufferResource = node->GetFramebufferResource();
            if (framebufferResource && framebufferResource->IsScheduled()) {
                framebufferResource->CancelScheduledDestroy();
            }

            auto* renderPassResource = node->GetRenderPassResource();
            if (renderPassResource && renderPassResource->IsScheduled()) {
                renderPassResource->CancelScheduledDestroy();
            }

            // Set FrameGraph reference and index for automatic resource management
            node->SetFrameGraph(this);
            node->SetIndex(index);

            // If node is an IRenderPass, automatically set execute callback from OnDraw
            // This eliminates the need for Pass to manually call SetExecuteCallback

            IRenderPass* pass = dynamic_cast<IRenderPass*>(node);
            if (pass) {
                // Create a lambda that calls OnDraw
                FrameGraphNode::ExecuteCallback callback = [pass](FrameGraphBuilder& builder, const RenderCommandList* sceneCommands) -> RenderCommandList {
                      return pass->OnDraw(builder, sceneCommands);
                    };
                node->SetExecuteCallback(callback);
            }

            // Store as raw pointer (lifetime managed by caller)
            // We need to use a custom deleter that does nothing, since we don't own the node
            // Use a no-op deleter function
            auto noop_deleter = [](FrameGraphNode*) {};
            std::unique_ptr<FrameGraphNode, std::function<void(FrameGraphNode*)>> nodePtr(node, noop_deleter);
            m_Nodes.push_back(std::move(nodePtr));
            m_NodeNameToIndex[name] = index;
            return node;
        }

        FrameGraphResource* FrameGraph::AddResource(const std::string& name, const ResourceDescription& desc) {
            auto it = m_Resources.find(name);
            if (it != m_Resources.end()) {
                // Resource already exists - check if description has changed
                auto* existingResource = it->second.get();
                const auto& existingDesc = existingResource->GetDescription();

                // Compare resource descriptions to detect changes
                bool descriptionChanged = false;
                if (existingDesc.GetType() != desc.GetType()) {
                    descriptionChanged = true;
                }
                else if (desc.GetType() == ResourceType::Texture || desc.GetType() == ResourceType::Attachment) {
                    if (existingDesc.GetWidth() != desc.GetWidth() ||
                        existingDesc.GetHeight() != desc.GetHeight() ||
                        existingDesc.GetFormat() != desc.GetFormat() ||
                        existingDesc.HasDepth() != desc.HasDepth()) {
                        descriptionChanged = true;
                    }
                }
                else if (desc.GetType() == ResourceType::Buffer) {
                    if (existingDesc.GetSize() != desc.GetSize() ||
                        existingDesc.GetBufferUsage() != desc.GetBufferUsage()) {
                        descriptionChanged = true;
                    }
                }

                if (descriptionChanged) {
                    // Resource description changed - need to recreate
                    // Release old resource first
                    ResourceType type = existingDesc.GetType();
                    if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                        RHI::IImage* image = existingResource->GetRHIImage();
                        if (image) {
                            delete image;
                            existingResource->SetRHIResource(static_cast<RHI::IImage*>(nullptr));
                        }
                    }
                    else if (type == ResourceType::Buffer) {
                        RHI::IBuffer* buffer = existingResource->GetRHIBuffer();
                        if (buffer) {
                            delete buffer;
                            existingResource->SetRHIResource(static_cast<RHI::IBuffer*>(nullptr));
                        }
                    }

                    // Update description (resource object is reused, only description changes)
                    // Note: FrameGraphResource stores description by value, so we need to replace the resource
                    auto newResource = std::make_unique<FrameGraphResource>(name, desc);
                    FrameGraphResource* resourcePtr = newResource.get();
                    m_Resources[name] = std::move(newResource);
                    return resourcePtr;
                }
                else {
                    // Description unchanged - reuse existing resource
                    return existingResource;
                }
            }

            // New resource - create it
            auto resource = std::make_unique<FrameGraphResource>(name, desc);
            FrameGraphResource* resourcePtr = resource.get();
            m_Resources[name] = std::move(resource);
            return resourcePtr;
        }

        bool FrameGraph::AllocateResource(const std::string& resourceName) {
            auto it = m_Resources.find(resourceName);
            if (it == m_Resources.end()) {
                return false;
            }

            auto* resource = it->second.get();
            const auto& desc = resource->GetDescription();

            // Check if already allocated
            ResourceType type = desc.GetType();
            if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                if (resource->GetRHIImage() != nullptr) {
                    return true; // Already allocated
                }
            }
            else if (type == ResourceType::Buffer) {
                if (resource->GetRHIBuffer() != nullptr) {
                    return true; // Already allocated
                }
            }

            // Allocate resource
            if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                // Create image
                RHI::ImageDescription imageDesc;
                imageDesc.width = desc.GetWidth();
                imageDesc.height = desc.GetHeight();
                imageDesc.format = desc.GetFormat();

                // Determine usage based on resource type and depth flag
                if (type == ResourceType::Attachment) {
                    if (desc.HasDepth()) {
                        imageDesc.usage = RHI::ImageUsageFlags::DepthStencilAttachment;
                    }
                    else {
                        imageDesc.usage = RHI::ImageUsageFlags::ColorAttachment;
                    }
                }
                else {
                    imageDesc.usage = RHI::ImageUsageFlags::Sampled;
                }
                imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                auto image = m_Device->CreateImage(imageDesc);
                if (!image) {
                    std::cerr << "Error: FrameGraph::AllocateResource: Failed to create image for resource '"
                        << resourceName << "' (width: " << desc.GetWidth()
                        << ", height: " << desc.GetHeight()
                        << ", format: " << static_cast<int>(desc.GetFormat()) << ")" << std::endl;
                    return false;
                }

                resource->SetRHIResource(image.release());
            }
            else if (type == ResourceType::Buffer) {
                // Create buffer
                auto buffer = m_Device->CreateBuffer(
                    desc.GetSize(),
                    desc.GetBufferUsage(),
                    RHI::MemoryPropertyFlags::DeviceLocal
                );
                if (!buffer) {
                    return false;
                }

                resource->SetRHIResource(buffer.release());
            }

            return true;
        }

        bool FrameGraph::BuildExecutionPlan(FrameGraphExecutionPlan& plan) {
            plan.Clear();

            // Analyze dependencies first
            AnalyzeDependencies();

            // Build node plans
            for (const auto& node : m_Nodes) {
                FrameGraphExecutionPlan::NodePlan nodePlan;
                nodePlan.name = node->GetName();
                nodePlan.index = node->GetIndex();
                nodePlan.type = node->GetType(); // RenderPassType enum
                nodePlan.typeString = node->GetTypeString(); // String representation
                nodePlan.readResources = node->GetReadResources();
                nodePlan.writeResources = node->GetWriteResources();
                nodePlan.dependencies = node->GetDependencies();
                plan.AddNodePlan(std::move(nodePlan));
            }

            // Build resource plans
            for (const auto& [resourceName, resource] : m_Resources) {
                FrameGraphExecutionPlan::ResourcePlan resourcePlan;
                resourcePlan.name = resource->GetName();
                const auto& desc = resource->GetDescription();
                resourcePlan.type = desc.GetType();

                // Clone the ResourceDescription based on type
                ResourceType type = desc.GetType();
                if (type == ResourceType::Attachment) {
                    resourcePlan.description = std::make_unique<AttachmentResource>(
                        desc.GetName(),
                        desc.GetWidth(),
                        desc.GetHeight(),
                        desc.GetFormat(),
                        desc.HasDepth()
                    );
                }
                else if (type == ResourceType::Buffer) {
                    resourcePlan.description = std::make_unique<BufferResource>(
                        desc.GetName(),
                        desc.GetSize(),
                        desc.GetBufferUsage()
                    );
                }
                else {
                    // Fallback: create a basic ResourceDescription
                    resourcePlan.description = std::make_unique<ResourceDescription>(type, desc.GetName());
                }

                plan.AddResourcePlan(std::move(resourcePlan));
            }

            // Calculate execution order (topological sort)
            std::vector<uint32_t> executionOrder = TopologicalSort();
            plan.SetExecutionOrder(executionOrder);

            return plan.IsValid();
        }

        bool FrameGraph::Compile(const FrameGraphExecutionPlan& plan) {
            if (!plan.IsValid()) {
                return false;
            }

            // Allocate resources based on the execution plan
            // Resources are allocated when needed, not during plan building
            return AllocateResources();
        }

        RenderCommandList FrameGraph::Execute(const FrameGraphExecutionPlan& plan, Resources::Scene* scene, const RenderConfig& renderConfig) {
            RenderCommandList commandList;

            if (!plan.IsValid()) {
                return commandList;
            }

            FrameGraphBuilder builder(this);

            // Execute nodes in the order specified by the plan
            // For each node that is an IRenderPass:
            //   1. If it has a SceneRenderer, call Render() to generate SceneRenderCommands
            //   2. Get SceneRenderCommands from SceneRenderer
            //   3. Pass SceneRenderCommands to OnDraw() callback

            const auto& executionOrder = plan.GetExecutionOrder();
            for (uint32_t nodeIndex : executionOrder) {
                auto* node = m_Nodes[nodeIndex].get();
                if (!node || !node->GetExecuteCallback()) {
                    continue;
                }

                // Check if we can reuse existing render pass and framebuffer
                // Only recreate if configuration has changed
                auto* existingFramebufferResource = node->GetFramebufferResource();
                auto* existingRenderPassResource = node->GetRenderPassResource();
                RHI::IRenderPass* nodeRenderPass = nullptr;
                RHI::IFramebuffer* nodeFramebuffer = nullptr;
                bool needRecreateRenderPass = true;
                bool needRecreateFramebuffer = true;

                // Create render pass for this node based on its write resources
                // This render pass will be used for pipeline creation
                std::vector<RHI::AttachmentDescription> attachments;
                bool hasDepth = false;

                // Collect attachments from write resources
                const auto& writeResources = node->GetWriteResources();
                if (writeResources.size() > 8) {
                    std::cerr << "Warning: Node '" << node->GetName()
                        << "' has " << writeResources.size()
                        << " write resources. This is likely a bug." << std::endl;
                    std::cerr << "First 10 write resources: ";
                    for (size_t i = 0; i < std::min(size_t(10), writeResources.size()); ++i) {
                        std::cerr << "'" << writeResources[i] << "' ";
                    }
                    std::cerr << std::endl;
                }

                for (const auto& writeResName : writeResources) {
                    auto* resource = GetResource(writeResName);
                    if (resource && resource->GetDescription().GetType() == ResourceType::Attachment) {
                        const auto& desc = resource->GetDescription();
                        RHI::AttachmentDescription attachment;
                        attachment.format = desc.GetFormat();
                        attachment.samples = 1;
                        attachment.loadOpClear = true;
                        attachment.storeOpStore = true;
                        attachment.stencilLoadOpClear = false;
                        attachment.stencilStoreOpStore = false;
                        attachment.initialLayout = RHI::Format::Undefined;
                        attachment.finalLayout = RHI::Format::Undefined;

                        if (desc.HasDepth()) {
                            hasDepth = true;
                        }
                        else {
                            attachments.push_back(attachment);
                        }
                    }
                }

                // Create render pass description and compare with existing
                RHI::RenderPassDescription renderPassDesc;
                if (!attachments.empty() || hasDepth) {
                    renderPassDesc.colorAttachments = attachments;
                    if (hasDepth) {
                        // Find depth attachment
                        for (const auto& writeResName : node->GetWriteResources()) {
                            auto* resource = GetResource(writeResName);
                            if (resource && resource->GetDescription().GetType() == ResourceType::Attachment) {
                                const auto& desc = resource->GetDescription();
                                if (desc.HasDepth()) {
                                    RHI::AttachmentDescription depthAttachment;
                                    depthAttachment.format = desc.GetFormat();
                                    depthAttachment.samples = 1;
                                    depthAttachment.loadOpClear = true;
                                    depthAttachment.storeOpStore = true;
                                    depthAttachment.stencilLoadOpClear = false;
                                    depthAttachment.stencilStoreOpStore = false;
                                    depthAttachment.initialLayout = RHI::Format::Undefined;
                                    depthAttachment.finalLayout = RHI::Format::Undefined;
                                    renderPassDesc.depthAttachment = depthAttachment;
                                    renderPassDesc.hasDepthAttachment = true;
                                    break;
                                }
                            }
                        }
                    }

                    // Check if existing render pass matches current configuration
                    // IMPORTANT: We need to check multiple conditions:
                    // 1. existingRenderPassResource exists and is created
                    // 2. The actual render pass pointer is still valid
                    // 3. The cached description matches the current description (if cached exists)
                    // 4. If no cache exists but render pass is valid, we can still reuse it and update cache
                    if (existingRenderPassResource && existingRenderPassResource->IsCreated()) {
                        RHI::IRenderPass* existingRenderPass = existingRenderPassResource->GetRenderPass();
                        if (existingRenderPass) {
                            // Render pass resource exists and render pass pointer is valid
                            const RHI::RenderPassDescription* cachedDesc = node->GetCachedRenderPassDescription();
                            if (cachedDesc) {
                                // We have a cached description - compare with current configuration
                                bool configMatches = true;
                                
                                // Compare color attachments
                                if (cachedDesc->colorAttachments.size() != renderPassDesc.colorAttachments.size()) {
                                    configMatches = false;
                                } else {
                                    for (size_t i = 0; i < renderPassDesc.colorAttachments.size(); ++i) {
                                        const auto& cached = cachedDesc->colorAttachments[i];
                                        const auto& current = renderPassDesc.colorAttachments[i];
                                        if (cached.format != current.format ||
                                            cached.samples != current.samples ||
                                            cached.loadOpClear != current.loadOpClear ||
                                            cached.storeOpStore != current.storeOpStore) {
                                            configMatches = false;
                                            break;
                                        }
                                    }
                                }
                                
                                // Compare depth attachment
                                if (configMatches) {
                                    if (cachedDesc->hasDepthAttachment != renderPassDesc.hasDepthAttachment) {
                                        configMatches = false;
                                    } else if (renderPassDesc.hasDepthAttachment) {
                                        const auto& cached = cachedDesc->depthAttachment;
                                        const auto& current = renderPassDesc.depthAttachment;
                                        if (cached.format != current.format ||
                                            cached.samples != current.samples ||
                                            cached.loadOpClear != current.loadOpClear ||
                                            cached.storeOpStore != current.storeOpStore) {
                                            configMatches = false;
                                        }
                                    }
                                }
                                
                                if (configMatches) {
                                    // Configuration matches - reuse existing render pass
                                    needRecreateRenderPass = false;
                                    nodeRenderPass = existingRenderPass;
                                } else {
                                    // Configuration changed - need to recreate
                                    needRecreateRenderPass = true;
                                }
                            } else {
                                // No cached description, but render pass exists and is valid
                                // This can happen on first frame or if cache was cleared
                                // For safety, we'll assume the render pass might not match the current config
                                // and recreate it, then cache the new description
                                // However, if we want to be more aggressive, we could reuse it and update cache
                                // For now, we'll recreate to ensure correctness
                                needRecreateRenderPass = true;
                            }
                        } else {
                            // Render pass resource exists but render pass pointer is null
                            // This shouldn't happen if IsCreated() is true, but handle it anyway
                            needRecreateRenderPass = true;
                        }
                    } else {
                        // No existing render pass resource or it's not created yet
                        // Need to create a new one
                        needRecreateRenderPass = true;
                    }

                    // Create render pass only if needed
                    if (needRecreateRenderPass) {
                        nodeRenderPass = m_Device->CreateRenderPass(renderPassDesc).release();
                        if (!nodeRenderPass) {
                            std::cerr << "Error: FrameGraph::Execute: Failed to create RenderPass for node '"
                                << node->GetName() << "'" << std::endl;
                        }
                    } else {
                        // Reuse existing render pass
                        // nodeRenderPass should already be set in the comparison logic above
                        // But ensure it's set here as well for safety
                        if (!nodeRenderPass && existingRenderPassResource) {
                            nodeRenderPass = existingRenderPassResource->GetRenderPass();
                        }
                    }
                }
                else {
                    // No attachments and no depth - this node doesn't need a render pass
                    // This is normal for compute passes or passes that only read resources
                }

                // Create framebuffer for this node (only if needed)
                if (needRecreateFramebuffer && nodeRenderPass && (!attachments.empty() || hasDepth)) {
                    // Collect image views from write resources
                    // IMPORTANT: Order matters! ImageViews must match RenderPass attachment order:
                    // 1. All color attachments first (in the same order as in attachments vector)
                    // 2. Depth attachment last (if hasDepth is true)
                    std::vector<RHI::IImageView*> imageViews;
                    RHI::IImageView* depthImageView = nullptr;
                    uint32_t width = 0;
                    uint32_t height = 0;

                    // First, collect color attachments (in the same order as attachments vector)
                    for (const auto& writeResName : writeResources) {
                        auto* resource = GetResource(writeResName);
                        if (!resource) {
                            std::cerr << "Warning: FrameGraph::Execute: Resource '" << writeResName
                                << "' not found for node '" << node->GetName() << "'" << std::endl;
                            continue;
                        }

                        if (resource->GetDescription().GetType() != ResourceType::Attachment) {
                            continue; // Skip non-attachment resources
                        }

                        const auto& desc = resource->GetDescription();
                        // Skip depth attachments in this pass - collect them separately
                        if (desc.HasDepth()) {
                            continue;
                        }

                        RHI::IImage* image = resource->GetRHIImage();
                        if (!image) {
                            std::cerr << "Warning: FrameGraph::Execute: Resource '" << writeResName
                                << "' has no RHI Image allocated for node '" << node->GetName() << "'" << std::endl;
                            continue;
                        }

                        // Create image view (or get existing one)
                        RHI::IImageView* imageView = image->CreateImageView();
                        if (imageView) {
                            // Note: CreateImageView() returns a pointer to an object owned by VulkanImage
                            // ImageView lifetime is tied to the Image, which is owned by FrameGraphResource
                            // Images are stored in m_Resources and will be kept alive until ReleaseResources()
                            // ReleaseResources() ensures framebuffers are destroyed before images,
                            // so ImageViews will stay valid while framebuffers exist
                            imageViews.push_back(imageView);
                            if (width == 0) {
                                width = desc.GetWidth();
                                height = desc.GetHeight();
                            }
                        }
                        else {
                            std::cerr << "Warning: FrameGraph::Execute: Failed to create ImageView for resource '"
                                << writeResName << "' in node '" << node->GetName() << "'" << std::endl;
                        }
                    }

                    // Then, collect depth attachment (if exists)
                    if (hasDepth) {
                        for (const auto& writeResName : writeResources) {
                            auto* resource = GetResource(writeResName);
                            if (!resource) {
                                continue;
                            }

                            if (resource->GetDescription().GetType() != ResourceType::Attachment) {
                                continue;
                            }

                            const auto& desc = resource->GetDescription();
                            if (desc.HasDepth()) {
                                RHI::IImage* image = resource->GetRHIImage();
                                if (!image) {
                                    std::cerr << "Warning: FrameGraph::Execute: Depth resource '" << writeResName
                                        << "' has no RHI Image allocated for node '" << node->GetName() << "'" << std::endl;
                                    continue;
                                }

                                depthImageView = image->CreateImageView();
                                if (depthImageView) {
                                    if (width == 0) {
                                        width = desc.GetWidth();
                                        height = desc.GetHeight();
                                    }
                                    break; // Only one depth attachment
                                }
                                else {
                                    std::cerr << "Warning: FrameGraph::Execute: Failed to create ImageView for depth resource '"
                                        << writeResName << "' in node '" << node->GetName() << "'" << std::endl;
                                }
                            }
                        }

                        // Add depth attachment at the end
                        if (depthImageView) {
                            imageViews.push_back(depthImageView);
                        }
                        else {
                            std::cerr << "Error: FrameGraph::Execute: Depth attachment expected but ImageView is nullptr for node '"
                                << node->GetName() << "'" << std::endl;
                        }
                    }

                    // Check if existing framebuffer matches current configuration
                    // IMPORTANT: We need to check multiple conditions:
                    // 1. existingFramebufferResource exists and is created
                    // 2. The actual framebuffer pointer is still valid
                    // 3. The framebuffer configuration (width, height, render pass) matches current requirements
                    if (existingFramebufferResource && existingFramebufferResource->IsCreated()) {
                        RHI::IFramebuffer* existingFramebuffer = existingFramebufferResource->GetFramebuffer();
                        if (existingFramebuffer) {
                            // Framebuffer resource exists and framebuffer pointer is valid
                            // Compare framebuffer configuration
                            // Check if width, height, and render pass match
                            // Note: We compare with nodeRenderPass (which may be reused or newly created)
                            if (existingFramebuffer->GetWidth() == width &&
                                existingFramebuffer->GetHeight() == height &&
                                nodeRenderPass != nullptr) {
                                // Check if render pass matches
                                // We need to get the render pass from the existing framebuffer's render pass resource
                                // For now, we'll assume if width/height match and render pass exists, it's likely the same
                                // A more robust check would compare the actual render pass pointers
                                if (existingRenderPassResource && 
                                    existingRenderPassResource->IsCreated() &&
                                    existingRenderPassResource->GetRenderPass() == nodeRenderPass) {
                                    // Configuration matches - reuse existing framebuffer
                                    nodeFramebuffer = existingFramebuffer;
                                    needRecreateFramebuffer = false;
                                } else {
                                    // Render pass doesn't match - need to recreate framebuffer
                                    needRecreateFramebuffer = true;
                                }
                            } else {
                                // Width/height don't match or render pass is null - need to recreate
                                needRecreateFramebuffer = true;
                            }
                        } else {
                            // Framebuffer resource exists but framebuffer pointer is null
                            // This shouldn't happen if IsCreated() is true, but handle it anyway
                            needRecreateFramebuffer = true;
                        }
                    } else {
                        // No existing framebuffer resource or it's not created yet
                        // Need to create a new one
                        needRecreateFramebuffer = true;
                    }

                    // Create framebuffer only if needed
                    if (needRecreateFramebuffer) {
                        // Expected count: color attachments + (depth if hasDepth)
                        uint32_t expectedAttachmentCount = static_cast<uint32_t>(attachments.size()) + (hasDepth ? 1 : 0);
                        if (imageViews.size() == expectedAttachmentCount && width > 0 && height > 0) {
                            nodeFramebuffer = m_Device->CreateFramebuffer(
                                nodeRenderPass,
                                imageViews,
                                width,
                                height
                            ).release();

                            if (!nodeFramebuffer) {
                                std::cerr << "Error: FrameGraph::Execute: Failed to create framebuffer for node '"
                                    << node->GetName() << "'" << std::endl;
                            }
                        }
                        else {
                            std::cerr << "Error: FrameGraph::Execute: Attachment count mismatch for framebuffer creation. "
                                << "Node: '" << node->GetName() << "', expected: " << expectedAttachmentCount
                                << ", got: " << imageViews.size()
                                << ", width: " << width << ", height: " << height << std::endl;
                            std::cerr << "  Color attachments: " << attachments.size()
                                << ", hasDepth: " << (hasDepth ? "true" : "false") << std::endl;
                        }
                    }
                }

                // Create FrameGraphBuilder with render pass and framebuffer
                FrameGraphBuilder builder(this, nodeRenderPass, nodeFramebuffer);

                // Check if node is an IRenderPass with SceneRenderer
                IRenderPass* pass = dynamic_cast<IRenderPass*>(node);
                const RenderCommandList* sceneCommands = nullptr;

                if (pass && pass->HasSceneRenderer() && scene) {
                    // Get SceneRenderer from pass
                    SceneRenderer* sceneRenderer = pass->GetSceneRenderer();

                    // Render scene using SceneRenderer with render pass for pipeline creation
                    // Camera config is automatically determined inside Render() from pass and renderConfig
                    // Pass nodeRenderPass so pipelines can be created during SubmitRenderQueue
                    sceneRenderer->Render(scene, pass, renderConfig, nodeRenderPass);

                    // Get generated commands from SceneRenderer
                    if (sceneRenderer->HasRenderCommands()) {
                        sceneCommands = &sceneRenderer->GetRenderCommands();
                    }
                }

                // Get command list from node callback, passing scene commands
                // Geometry and forward passes can merge scene commands into their pass
                RenderCommandList nodeCommands = node->GetExecuteCallback()(builder, sceneCommands);

                // Debug: Check if commands were generated
                if (nodeCommands.IsEmpty()) {
                    std::cerr << "Warning: FrameGraph::Execute: Node '" << node->GetName()
                        << "' generated empty command list" << std::endl;
                    if (!nodeRenderPass) {
                        std::cerr << "  - RenderPass is nullptr" << std::endl;
                    }
                    if (!nodeFramebuffer) {
                        std::cerr << "  - Framebuffer is nullptr" << std::endl;
                    }
                    if (!node->GetExecuteCallback()) {
                        std::cerr << "  - ExecuteCallback is not set" << std::endl;
                    }
                }

                // Store framebuffer and render pass in the node (managed through IRenderResource)
                // Only create new resource wrappers if we created new RHI objects
                if (needRecreateFramebuffer && nodeFramebuffer) {
                    // Create new framebuffer resource wrapper
                    auto framebufferResource = std::make_unique<FramebufferResource>();
                    framebufferResource->SetFramebuffer(nodeFramebuffer);
                    // Set device for proper destruction later
                    if (m_Device) {
                        framebufferResource->ScheduleCreate();
                        framebufferResource->ProcessScheduled(m_Device); // This sets m_Device
                        // Since DoCreate returns true (framebuffer already exists), it will be marked as Created
                    }
                    node->SetFramebufferResource(std::move(framebufferResource));
                } else if (nodeFramebuffer && existingFramebufferResource) {
                    // Reusing existing framebuffer - ensure device is set
                    if (m_Device && !existingFramebufferResource->GetDevice()) {
                        existingFramebufferResource->ScheduleCreate();
                        existingFramebufferResource->ProcessScheduled(m_Device);
                    }
                }

                if (needRecreateRenderPass && nodeRenderPass) {
                    // Create new render pass resource wrapper
                    auto renderPassResource = std::make_unique<RenderPassResource>();
                    renderPassResource->SetRenderPass(nodeRenderPass);
                    // Set device for proper destruction later
                    if (m_Device) {
                        renderPassResource->ScheduleCreate();
                        renderPassResource->ProcessScheduled(m_Device); // This sets m_Device
                    }
                    node->SetRenderPassResource(std::move(renderPassResource));
                    
                    // Cache the render pass description for future comparison
                    node->SetCachedRenderPassDescription(renderPassDesc);
                } else if (nodeRenderPass && existingRenderPassResource) {
                    // Reusing existing render pass - ensure device is set
                    if (m_Device && !existingRenderPassResource->GetDevice()) {
                        existingRenderPassResource->ScheduleCreate();
                        existingRenderPassResource->ProcessScheduled(m_Device);
                    }
                    
                    // Update cached description if it doesn't exist (shouldn't happen, but be safe)
                    if (!node->GetCachedRenderPassDescription()) {
                        node->SetCachedRenderPassDescription(renderPassDesc);
                    }
                } else if (nodeRenderPass) {
                    // Render pass was created but no resource wrapper exists yet (shouldn't happen)
                    // Cache the description anyway
                    node->SetCachedRenderPassDescription(renderPassDesc);
                }

                // Merge node commands into main command list
                const auto& nodeCmdList = nodeCommands.GetCommands();
                for (const auto& cmd : nodeCmdList) {
                    commandList.AddCommand(cmd);
                }
            }

            return commandList;
        }

        FrameGraphResource* FrameGraph::GetResource(const std::string& name) {
            auto it = m_Resources.find(name);
            return (it != m_Resources.end()) ? it->second.get() : nullptr;
        }

        const FrameGraphResource* FrameGraph::GetResource(const std::string& name) const {
            auto it = m_Resources.find(name);
            return (it != m_Resources.end()) ? it->second.get() : nullptr;
        }

        FrameGraphNode* FrameGraph::GetNode(uint32_t index) {
            if (index < m_Nodes.size()) {
                return m_Nodes[index].get();
            }
            return nullptr;
        }

        const FrameGraphNode* FrameGraph::GetNode(uint32_t index) const {
            if (index < m_Nodes.size()) {
                return m_Nodes[index].get();
            }
            return nullptr;
        }

        void FrameGraph::Clear() {
            // IMPORTANT: Clear() only clears the graph structure (nodes and indices), NOT the resources
            // Resources are kept alive across frames to avoid unnecessary recreation
            // Resources will be automatically reused if their description hasn't changed (see AddResource)
            // Resources will only be destroyed when:
            //   1. FrameGraph is destroyed (via ReleaseResources())
            //   2. Resource description changes (handled in AddResource)
            //   3. Explicitly calling ReleaseResources()

            m_Nodes.clear();
            // DO NOT clear m_Resources here - they are reused across frames
            // m_Resources.clear(); // <-- This causes resources to be recreated every frame!
            m_ResourceNameToIndex.clear();
            m_NodeNameToIndex.clear();
        }

        void FrameGraph::MarkRemovedNodesForDestruction() {
            // Schedule destruction for resources in nodes that are about to be removed
            // This is called before Clear(), so we can still access the nodes
            for (const auto& node : m_Nodes) {
                if (node) {
                    // Schedule destruction for framebuffer and render pass resources
                    // They will be destroyed in ProcessResources after the frame is submitted
                    auto* framebufferResource = node->GetFramebufferResource();
                    if (framebufferResource && framebufferResource->IsCreated()) {
                        framebufferResource->ScheduleDestroy();
                    }

                    auto* renderPassResource = node->GetRenderPassResource();
                    if (renderPassResource && renderPassResource->IsCreated()) {
                        renderPassResource->ScheduleDestroy();
                    }
                }
            }
        }

        void FrameGraph::ReleaseResources() {
            // IMPORTANT: Release order matters!
            // This method is used for cleanup when FrameGraph is being destroyed or when explicitly called
            // For per-frame resource management, use MarkRemovedNodesForDestruction() instead

            // Schedule destruction for all node resources
            MarkRemovedNodesForDestruction();

            // Release all allocated RHI resources (Images and Buffers)
            // Resources are managed by unique_ptr, so we need to delete them manually
            for (auto& [resourceName, resource] : m_Resources) {
                if (resource) {
                    const auto& desc = resource->GetDescription();
                    ResourceType type = desc.GetType();

                    if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                        // Release image
                        RHI::IImage* image = resource->GetRHIImage();
                        if (image) {
                            // Delete the image (destructor will handle cleanup)
                            // Note: This will also destroy any ImageViews owned by the image
                            delete image;
                            resource->SetRHIResource(static_cast<RHI::IImage*>(nullptr));
                        }
                    }
                    else if (type == ResourceType::Buffer) {
                        // Release buffer
                        RHI::IBuffer* buffer = resource->GetRHIBuffer();
                        if (buffer) {
                            // Delete the buffer (destructor will handle cleanup)
                            delete buffer;
                            resource->SetRHIResource(static_cast<RHI::IBuffer*>(nullptr));
                        }
                    }
                }
            }

            // Clear all resources (after releasing RHI objects)
            m_Resources.clear();

            // Note: Framebuffers and RenderPasses are now stored in FrameGraphNode
            // They are managed through IRenderResource lifecycle and will be destroyed
            // when nodes are destroyed or when ScheduleDestroy is called
        }

        void FrameGraph::AnalyzeDependencies() {

            for (auto& [resourceName, resource] : m_Resources) {
                uint32_t firstUse = UINT32_MAX;
                uint32_t lastUse = 0;

                for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                    FrameGraphNode* node = m_Nodes[i].get();
                    bool used = false;

                    // Check read resources
                    const std::vector<std::string>& readResources = node->GetReadResources();
                    for (const std::string& readRes : readResources) {
                        if (readRes == resourceName) {
                            used = true;
                            break;
                        }
                    }

                    // Check write resources
                    if (!used) {
                        const std::vector<std::string>& writeResources = node->GetWriteResources();
                        for (const std::string& writeRes : writeResources) {
                            if (writeRes == resourceName) {
                                used = true;
                                break;
                            }
                        }
                    }

                    if (used) {
                        firstUse = std::min(firstUse, i);
                        lastUse = std::max(lastUse, i);
                    }
                }

                // Update resource description

                auto& desc = const_cast<ResourceDescription&>(resource->GetDescription());
                desc.SetFirstPass(firstUse);
                desc.SetLastPass(lastUse);
            }

            // Build dependencies between nodes
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                auto* node = m_Nodes[i].get();

                // For each written resource, find the last node that wrote to it
                for (const auto& writeRes : node->GetWriteResources()) {
                    auto* resource = GetResource(writeRes);
                    if (resource) {
                        // Find all subsequent nodes that read this resource
                        for (uint32_t j = i + 1; j < m_Nodes.size(); ++j) {
                            auto* otherNode = m_Nodes[j].get();
                            for (const auto& readRes : otherNode->GetReadResources()) {
                                if (readRes == writeRes) {
                                    otherNode->AddDependency(i);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }

        bool FrameGraph::AllocateResources() {
            // Allocate actual RHI resources for each resource
            // Note: Resources may have already been allocated by individual Passes
            // This method will skip already-allocated resources
            for (auto& [resourceName, resource] : m_Resources) {
                const auto& desc = resource->GetDescription();
                ResourceType type = desc.GetType();

                // Check if already allocated
                bool alreadyAllocated = false;
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    if (resource->GetRHIImage() != nullptr) {
                        alreadyAllocated = true;
                    }
                }
                else if (type == ResourceType::Buffer) {
                    if (resource->GetRHIBuffer() != nullptr) {
                        alreadyAllocated = true;
                    }
                }

                if (alreadyAllocated) {
                    continue; // Skip already allocated resources
                }

                // Allocate resource
                if (type == ResourceType::Texture || type == ResourceType::Attachment) {
                    // Create image
                    RHI::ImageDescription imageDesc;
                    imageDesc.width = desc.GetWidth();
                    imageDesc.height = desc.GetHeight();
                    imageDesc.format = desc.GetFormat();

                    // Determine usage based on resource type and depth flag
                    if (type == ResourceType::Attachment) {
                        if (desc.HasDepth()) {
                            imageDesc.usage = RHI::ImageUsageFlags::DepthStencilAttachment;
                        }
                        else {
                            imageDesc.usage = RHI::ImageUsageFlags::ColorAttachment;
                        }
                    }
                    else {
                        imageDesc.usage = RHI::ImageUsageFlags::Sampled;
                    }
                    imageDesc.memoryProperties = RHI::MemoryPropertyFlags::DeviceLocal;

                    auto image = m_Device->CreateImage(imageDesc);
                    if (!image) {
                        std::cerr << "Error: FrameGraph::AllocateResources: Failed to create image for resource '"
                            << resourceName << "' (width: " << desc.GetWidth()
                            << ", height: " << desc.GetHeight()
                            << ", format: " << static_cast<int>(desc.GetFormat()) << ")" << std::endl;
                        return false;
                    }

                    // Store image pointer, FrameGraph manages the lifecycle
                    resource->SetRHIResource(image.release());
                }
                else if (type == ResourceType::Buffer) {
                    // Create buffer
                    auto buffer = m_Device->CreateBuffer(
                        desc.GetSize(),
                        desc.GetBufferUsage(),
                        RHI::MemoryPropertyFlags::DeviceLocal
                    );
                    if (!buffer) {
                        return false;
                    }

                    // Store buffer pointer, FrameGraph manages the lifecycle
                    resource->SetRHIResource(buffer.release());
                }
            }

            return true;
        }

        std::vector<uint32_t> FrameGraph::TopologicalSort() {
            std::vector<uint32_t> result;
            std::vector<int> inDegree(m_Nodes.size(), 0);

            // Calculate in-degree
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                auto* node = m_Nodes[i].get();
                for (uint32_t dep : node->GetDependencies()) {
                    inDegree[i]++;
                }
            }

            // Kahn's algorithm
            std::queue<uint32_t> queue;
            for (uint32_t i = 0; i < m_Nodes.size(); ++i) {
                if (inDegree[i] == 0) {
                    queue.push(i);
                }
            }

            while (!queue.empty()) {
                uint32_t u = queue.front();
                queue.pop();
                result.push_back(u);

                for (uint32_t v = 0; v < m_Nodes.size(); ++v) {
                    if (v == u) continue;
                    auto* node = m_Nodes[v].get();
                    for (uint32_t dep : node->GetDependencies()) {
                        if (dep == u) {
                            inDegree[v]--;
                            if (inDegree[v] == 0) {
                                queue.push(v);
                            }
                        }
                    }
                }
            }

            if (result.size() != m_Nodes.size()) {
                throw std::runtime_error("FrameGraph contains cycles!");
            }

            return result;
        }

    } // namespace Renderer
} // namespace FirstEngine
