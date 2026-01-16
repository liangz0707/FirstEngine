#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include <cstdint>
#include <atomic>
#include <mutex>

namespace FirstEngine {
    namespace Renderer {

        // Resource creation state (thread-safe)
        enum class ResourceState : uint32_t {
            Uninitialized = 0,  // Not yet initialized
            ScheduledCreate,    // Scheduled for creation (waiting to be processed)
            Creating,           // Currently being created
            Created,            // Successfully created
            ScheduledUpdate,    // Scheduled for update
            Updating,           // Currently being updated
            ScheduledDestroy,   // Scheduled for destruction
            Destroying,         // Currently being destroyed
            Destroyed           // Destroyed
        };

        // Abstract interface for render resources
        // Manages the lifecycle of GPU resources (buffers, images, shaders, etc.)
        // Thread-safe state management with scheduling support for frame-by-frame processing
        class FE_RENDERER_API IRenderResource {
        public:
            IRenderResource();
            virtual ~IRenderResource() = default;

            // Thread-safe state queries
            ResourceState GetState() const;
            bool IsCreated() const;
            bool IsInitialized() const;
            bool IsScheduled() const; // Returns true if scheduled for create/update/destroy

            // Scheduling interface - thread-safe, can be called from any thread
            // These methods schedule operations that will be processed in OnCreateResources()
            void ScheduleCreate();   // Schedule resource creation
            void ScheduleUpdate();   // Schedule resource update
            void ScheduleDestroy();  // Schedule resource destruction

            // Process scheduled operations (called from OnCreateResources, single-threaded)
            // Returns true if operation was processed, false if no operation scheduled
            bool ProcessScheduled(RHI::IDevice* device);

            // Get device (set during DoCreate)
            RHI::IDevice* GetDevice() const { return m_Device; }

        protected:
            // Virtual methods for actual resource operations (called from ProcessScheduled)
            // These are called on the render thread during OnCreateResources
            virtual bool DoCreate(RHI::IDevice* device) = 0;  // Create GPU resources
            virtual bool DoUpdate(RHI::IDevice* device) = 0; // Update GPU resources
            virtual void DoDestroy() = 0;                    // Destroy GPU resources

            // Helper to set state (thread-safe)
            void SetState(ResourceState state);

            // Device access (protected, set during ProcessScheduled)
            RHI::IDevice* m_Device;

        private:
            // Thread-safe state management
            mutable std::atomic<ResourceState> m_State;
            mutable std::mutex m_StateMutex; // For complex state transitions (currently unused, reserved for future use)
        };

    } // namespace Renderer
} // namespace FirstEngine
