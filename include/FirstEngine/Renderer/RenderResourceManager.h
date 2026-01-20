#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/IRenderResource.h"
#include "FirstEngine/RHI/IDevice.h"
#include <vector>
#include <mutex>
#include <memory>
#include <cstdint>

namespace FirstEngine {
    namespace Renderer {

        // RenderResourceManager - manages all IRenderResources
        // Thread-safe collection and processing of render resources
        // Supports frame-by-frame resource creation for performance
        // Singleton pattern for global access
        class FE_RENDERER_API RenderResourceManager {
        public:
            // Singleton access
            static RenderResourceManager& GetInstance();
            static void Initialize();
            static void Shutdown();

            // Register a resource (thread-safe)
            // Resources should register themselves when created
            void RegisterResource(IRenderResource* resource);

            // Unregister a resource (thread-safe)
            // Resources should unregister themselves when destroyed
            void UnregisterResource(IRenderResource* resource);

            // Process scheduled resources (called from OnCreateResources)
            // Processes a limited number of resources per frame for performance
            // maxResourcesPerFrame: Maximum number of resources to process this frame (0 = unlimited)
            // Returns number of resources processed
            uint32_t ProcessScheduledResources(RHI::IDevice* device, uint32_t maxResourcesPerFrame = 0);

            // Get all registered resources (for debugging/inspection)
            std::vector<IRenderResource*> GetAllResources() const;

            // Get count of resources in each state
            struct ResourceStatistics {
                uint32_t uninitialized = 0;
                uint32_t scheduledCreate = 0;
                uint32_t creating = 0;
                uint32_t created = 0;
                uint32_t scheduledUpdate = 0;
                uint32_t updating = 0;
                uint32_t scheduledDestroy = 0;
                uint32_t destroying = 0;
                uint32_t destroyed = 0;
            };
            ResourceStatistics GetStatistics() const;

        private:
            RenderResourceManager() = default;
            
        public:
            ~RenderResourceManager() = default;
            
        private:

            // Delete copy constructor and assignment operator
            RenderResourceManager(const RenderResourceManager&) = delete;
            RenderResourceManager& operator=(const RenderResourceManager&) = delete;

            static std::unique_ptr<RenderResourceManager> s_Instance;

            mutable std::mutex m_ResourcesMutex;
            std::vector<IRenderResource*> m_Resources;
        };

    } // namespace Renderer
} // namespace FirstEngine
