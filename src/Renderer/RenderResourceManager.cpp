#include "FirstEngine/Renderer/RenderResourceManager.h"
#include <algorithm>
#include <memory>

namespace FirstEngine {
    namespace Renderer {

        // Singleton instance
        std::unique_ptr<RenderResourceManager> RenderResourceManager::s_Instance = nullptr;

        RenderResourceManager& RenderResourceManager::GetInstance() {
            if (!s_Instance) {
                Initialize();
            }
            return *s_Instance;
        }

        void RenderResourceManager::Initialize() {
            if (!s_Instance) {
                s_Instance = std::unique_ptr<RenderResourceManager>(new RenderResourceManager());
            }
        }

        void RenderResourceManager::Shutdown() {
            if (s_Instance) {
                s_Instance.reset();
            }
        }

        void RenderResourceManager::RegisterResource(IRenderResource* resource) {
            if (!resource) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            // Check if already registered
            auto it = std::find(m_Resources.begin(), m_Resources.end(), resource);
            if (it == m_Resources.end()) {
                m_Resources.push_back(resource);
            }
        }

        void RenderResourceManager::UnregisterResource(IRenderResource* resource) {
            if (!resource) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            auto it = std::find(m_Resources.begin(), m_Resources.end(), resource);
            if (it != m_Resources.end()) {
                m_Resources.erase(it);
            }
        }

        uint32_t RenderResourceManager::ProcessScheduledResources(
            RHI::IDevice* device, 
            uint32_t maxResourcesPerFrame
        ) {
            if (!device) {
                return 0;
            }

            // Get a snapshot of resources (thread-safe)
            std::vector<IRenderResource*> resources;
            {
                std::lock_guard<std::mutex> lock(m_ResourcesMutex);
                resources = m_Resources;
            }

            uint32_t processedCount = 0;
            uint32_t processedThisFrame = 0;

            // Process scheduled resources
            for (IRenderResource* resource : resources) {
                if (!resource) {
                    continue;
                }

                // Check if resource is scheduled
                if (!resource->IsScheduled()) {
                    continue;
                }

                // Process the scheduled operation
                if (resource->ProcessScheduled(device)) {
                    processedCount++;
                    processedThisFrame++;

                    // Limit resources processed per frame
                    if (maxResourcesPerFrame > 0 && processedThisFrame >= maxResourcesPerFrame) {
                        break;
                    }
                }
            }

            return processedCount;
        }

        std::vector<IRenderResource*> RenderResourceManager::GetAllResources() const {
            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            return m_Resources;
        }

        RenderResourceManager::ResourceStatistics RenderResourceManager::GetStatistics() const {
            ResourceStatistics stats;

            std::lock_guard<std::mutex> lock(m_ResourcesMutex);
            
            for (IRenderResource* resource : m_Resources) {
                if (!resource) {
                    continue;
                }

                ResourceState state = resource->GetState();
                switch (state) {
                    case ResourceState::Uninitialized:
                        stats.uninitialized++;
                        break;
                    case ResourceState::ScheduledCreate:
                        stats.scheduledCreate++;
                        break;
                    case ResourceState::Creating:
                        stats.creating++;
                        break;
                    case ResourceState::Created:
                        stats.created++;
                        break;
                    case ResourceState::ScheduledUpdate:
                        stats.scheduledUpdate++;
                        break;
                    case ResourceState::Updating:
                        stats.updating++;
                        break;
                    case ResourceState::ScheduledDestroy:
                        stats.scheduledDestroy++;
                        break;
                    case ResourceState::Destroying:
                        stats.destroying++;
                        break;
                    case ResourceState::Destroyed:
                        stats.destroyed++;
                        break;
                }
            }

            return stats;
        }

    } // namespace Renderer
} // namespace FirstEngine
