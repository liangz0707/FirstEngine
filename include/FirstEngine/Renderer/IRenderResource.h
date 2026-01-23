#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/RHI/IDevice.h"
#include <cstdint>
#include <atomic>
#include <mutex>

namespace FirstEngine {
    namespace Renderer {

        // Actual resource state - represents the actual state of the resource
        enum class ResourceState : uint32_t {
            Uninitialized = 0,  // Resource not yet created
            Created,            // Resource successfully created and ready to use
            Destroyed           // Resource has been destroyed
        };

        // Scheduled operation state - represents what operation is scheduled to be performed
        enum class ScheduleState : uint32_t {
            None = 0,              // No operation scheduled
            ScheduledCreate,       // Creation is scheduled
            ScheduledUpdate,       // Update is scheduled
            ScheduledDestroy       // Destruction is scheduled
        };

        // Operation state - represents what operation is currently being performed
        enum class OperationState : uint32_t {
            Idle = 0,              // No operation in progress
            Creating,              // Currently creating the resource
            Updating,              // Currently updating the resource
            Destroying             // Currently destroying the resource
        };

        // Abstract interface for render resources
        // Manages the lifecycle of GPU resources (buffers, images, shaders, etc.)
        // Thread-safe state management with scheduling support for frame-by-frame processing
        // 
        // State Machine Design:
        // - ResourceState: Actual state of the resource (Uninitialized, Created, Destroyed)
        // - ScheduleState: What operation is scheduled (None, ScheduledCreate, ScheduledUpdate, ScheduledDestroy)
        // - OperationState: What operation is currently executing (Idle, Creating, Updating, Destroying)
        //
        // State Transition Rules:
        // 1. ScheduleCreate: Only allowed when ResourceState is Uninitialized or Destroyed, and ScheduleState is None
        // 2. ScheduleUpdate: Only allowed when ResourceState is Created, and ScheduleState is None
        // 3. ScheduleDestroy: Only allowed when ResourceState is not Destroyed, and ScheduleState is None
        // 4. ProcessScheduled: Executes the scheduled operation based on ScheduleState, then clears ScheduleState
        class FE_RENDERER_API IRenderResource {
        public:
            IRenderResource();
            virtual ~IRenderResource() = default;

            // Thread-safe state queries
            ResourceState GetResourceState() const;
            ScheduleState GetScheduleState() const;
            OperationState GetOperationState() const;

            // Convenience queries
            bool IsCreated() const;           // Returns true if ResourceState == Created
            bool IsInitialized() const;      // Returns true if ResourceState != Uninitialized
            bool IsScheduled() const;         // Returns true if ScheduleState != None
            bool IsOperationInProgress() const; // Returns true if OperationState != Idle

            // Scheduling interface - thread-safe, can be called from any thread
            // These methods schedule operations that will be processed in ProcessScheduled()
            // Each method checks the current ResourceState and ScheduleState to determine if scheduling is allowed
            bool ScheduleCreate();   // Schedule resource creation (only if Uninitialized/Destroyed and not scheduled)
            bool ScheduleUpdate();   // Schedule resource update (only if Created and not scheduled)
            bool ScheduleDestroy();  // Schedule resource destruction (only if not Destroyed and not scheduled)

            // Cancel scheduled operation (e.g. when resource is reused across frames)
            // Only has effect when ScheduleState is ScheduledDestroy; then sets ScheduleState to None
            void CancelScheduledDestroy();

            // Process scheduled operations (called from OnCreateResources, single-threaded)
            // Returns true if an operation was processed, false if no operation scheduled
            // This method:
            // 1. Checks ScheduleState to determine what operation to perform
            // 2. Sets OperationState to the appropriate value
            // 3. Calls the corresponding Do* method
            // 4. Updates ResourceState based on operation result
            // 5. Clears ScheduleState and OperationState
            bool ProcessScheduled(RHI::IDevice* device);

            // Get device (set during DoCreate)
            RHI::IDevice* GetDevice() const { return m_Device; }

        protected:
            // Virtual methods for actual resource operations (called from ProcessScheduled)
            // These are called on the render thread during ProcessScheduled
            virtual bool DoCreate(RHI::IDevice* device) = 0;  // Create GPU resources
            virtual bool DoUpdate(RHI::IDevice* device) = 0; // Update GPU resources
            virtual void DoDestroy() = 0;                    // Destroy GPU resources

            // Helper to set states (thread-safe, protected for derived classes)
            void SetResourceState(ResourceState state);
            void SetScheduleState(ScheduleState state);
            void SetOperationState(OperationState state);

            // Device access (protected, set during ProcessScheduled)
            RHI::IDevice* m_Device;

        private:
            // Thread-safe state management
            mutable std::atomic<ResourceState> m_ResourceState;
            mutable std::atomic<ScheduleState> m_ScheduleState;
            mutable std::atomic<OperationState> m_OperationState;
            mutable std::mutex m_StateMutex; // For complex state transitions (currently unused, reserved for future use)
        };

    } // namespace Renderer
} // namespace FirstEngine
