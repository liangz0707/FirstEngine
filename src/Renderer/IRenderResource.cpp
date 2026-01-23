#include "FirstEngine/Renderer/IRenderResource.h"
#include <atomic>

namespace FirstEngine {
    namespace Renderer {

        IRenderResource::IRenderResource() 
            : m_ResourceState(ResourceState::Uninitialized)
            , m_ScheduleState(ScheduleState::None)
            , m_OperationState(OperationState::Idle)
            , m_Device(nullptr) {
        }

        ResourceState IRenderResource::GetResourceState() const {
            return m_ResourceState.load(std::memory_order_acquire);
        }

        ScheduleState IRenderResource::GetScheduleState() const {
            return m_ScheduleState.load(std::memory_order_acquire);
        }

        OperationState IRenderResource::GetOperationState() const {
            return m_OperationState.load(std::memory_order_acquire);
        }

        bool IRenderResource::IsCreated() const {
            return m_ResourceState.load(std::memory_order_acquire) == ResourceState::Created;
        }

        bool IRenderResource::IsInitialized() const {
            ResourceState state = m_ResourceState.load(std::memory_order_acquire);
            return state != ResourceState::Uninitialized;
        }

        bool IRenderResource::IsScheduled() const {
            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);
            return scheduleState != ScheduleState::None;
        }

        bool IRenderResource::IsOperationInProgress() const {
            OperationState opState = m_OperationState.load(std::memory_order_acquire);
            return opState != OperationState::Idle;
        }

        bool IRenderResource::ScheduleCreate() {
            ResourceState resourceState = m_ResourceState.load(std::memory_order_acquire);
            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);

            // Only schedule create if:
            // 1. Resource is Uninitialized or Destroyed (can be recreated)
            // 2. No operation is currently scheduled
            if ((resourceState == ResourceState::Uninitialized || resourceState == ResourceState::Destroyed) &&
                scheduleState == ScheduleState::None) {
                m_ScheduleState.store(ScheduleState::ScheduledCreate, std::memory_order_release);
                return true;
            }
            return false;
        }

        bool IRenderResource::ScheduleUpdate() {
            ResourceState resourceState = m_ResourceState.load(std::memory_order_acquire);
            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);

            // Only schedule update if:
            // 1. Resource is Created (must exist to be updated)
            // 2. No operation is currently scheduled
            if (resourceState == ResourceState::Created &&
                scheduleState == ScheduleState::None) {
                m_ScheduleState.store(ScheduleState::ScheduledUpdate, std::memory_order_release);
                return true;
            }
            return false;
        }

        bool IRenderResource::ScheduleDestroy() {
            ResourceState resourceState = m_ResourceState.load(std::memory_order_acquire);
            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);

            // Only schedule destroy if:
            // 1. Resource is not already Destroyed
            // 2. No operation is currently scheduled
            if (resourceState != ResourceState::Destroyed &&
                scheduleState == ScheduleState::None) {
                m_ScheduleState.store(ScheduleState::ScheduledDestroy, std::memory_order_release);
                return true;
            }
            return false;
        }

        void IRenderResource::CancelScheduledDestroy() {
            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);
            if (scheduleState == ScheduleState::ScheduledDestroy) {
                m_ScheduleState.store(ScheduleState::None, std::memory_order_release);
            }
        }

        bool IRenderResource::ProcessScheduled(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            ScheduleState scheduleState = m_ScheduleState.load(std::memory_order_acquire);
            
            // No operation scheduled
            if (scheduleState == ScheduleState::None) {
                return false;
            }

            // Process scheduled create
            if (scheduleState == ScheduleState::ScheduledCreate) {
                // Clear schedule state first to prevent re-scheduling during operation
                m_ScheduleState.store(ScheduleState::None, std::memory_order_release);
                m_OperationState.store(OperationState::Creating, std::memory_order_release);
                
                // Set device
                m_Device = device;
                
                // Perform creation
                bool success = DoCreate(device);
                
                if (success) {
                    m_ResourceState.store(ResourceState::Created, std::memory_order_release);
                } else {
                    // Creation failed, reset to uninitialized
                    m_ResourceState.store(ResourceState::Uninitialized, std::memory_order_release);
                    m_Device = nullptr;
                }
                
                // Clear operation state
                m_OperationState.store(OperationState::Idle, std::memory_order_release);
                return true;
            }
            
            // Process scheduled update
            if (scheduleState == ScheduleState::ScheduledUpdate) {
                // Clear schedule state first
                m_ScheduleState.store(ScheduleState::None, std::memory_order_release);
                m_OperationState.store(OperationState::Updating, std::memory_order_release);
                
                // Perform update
                bool success = DoUpdate(device);
                
                // Update operation doesn't change resource state (stays Created)
                // If update fails, resource might still be usable, so we keep it as Created
                (void)success; // Suppress unused variable warning
                
                // Clear operation state
                m_OperationState.store(OperationState::Idle, std::memory_order_release);
                return true;
            }
            
            // Process scheduled destroy
            if (scheduleState == ScheduleState::ScheduledDestroy) {
                // Clear schedule state first
                m_ScheduleState.store(ScheduleState::None, std::memory_order_release);
                m_OperationState.store(OperationState::Destroying, std::memory_order_release);
                
                // Perform destruction
                DoDestroy();
                
                // Update resource state
                m_ResourceState.store(ResourceState::Destroyed, std::memory_order_release);
                m_Device = nullptr;
                
                // Clear operation state
                m_OperationState.store(OperationState::Idle, std::memory_order_release);
                return true;
            }
            
            return false; // Unknown schedule state (should not happen)
        }

        void IRenderResource::SetResourceState(ResourceState state) {
            m_ResourceState.store(state, std::memory_order_release);
        }

        void IRenderResource::SetScheduleState(ScheduleState state) {
            m_ScheduleState.store(state, std::memory_order_release);
        }

        void IRenderResource::SetOperationState(OperationState state) {
            m_OperationState.store(state, std::memory_order_release);
        }

    } // namespace Renderer
} // namespace FirstEngine
