#include "FirstEngine/Renderer/IRenderResource.h"
#include <atomic>

namespace FirstEngine {
    namespace Renderer {

        ResourceState IRenderResource::GetState() const {
            return m_State.load(std::memory_order_acquire);
        }

        bool IRenderResource::IsCreated() const {
            return m_State.load(std::memory_order_acquire) == ResourceState::Created;
        }

        bool IRenderResource::IsInitialized() const {
            ResourceState state = m_State.load(std::memory_order_acquire);
            return state != ResourceState::Uninitialized && 
                   state != ResourceState::ScheduledCreate &&
                   state != ResourceState::Destroyed;
        }

        bool IRenderResource::IsScheduled() const {
            ResourceState state = m_State.load(std::memory_order_acquire);
            return state == ResourceState::ScheduledCreate ||
                   state == ResourceState::ScheduledUpdate ||
                   state == ResourceState::ScheduledDestroy;
        }

        void IRenderResource::ScheduleCreate() {
            ResourceState currentState = m_State.load(std::memory_order_acquire);
            
            // Only schedule if not already created or scheduled
            if (currentState == ResourceState::Uninitialized || 
                currentState == ResourceState::Destroyed) {
                m_State.store(ResourceState::ScheduledCreate, std::memory_order_release);
            }
        }

        void IRenderResource::ScheduleUpdate() {
            ResourceState currentState = m_State.load(std::memory_order_acquire);
            
            // Only schedule if resource is already created
            if (currentState == ResourceState::Created) {
                m_State.store(ResourceState::ScheduledUpdate, std::memory_order_release);
            }
        }

        void IRenderResource::ScheduleDestroy() {
            ResourceState currentState = m_State.load(std::memory_order_acquire);
            
            // Schedule destroy from any state except already destroyed or destroying
            if (currentState != ResourceState::Destroyed && 
                currentState != ResourceState::Destroying) {
                m_State.store(ResourceState::ScheduledDestroy, std::memory_order_release);
            }
        }

        bool IRenderResource::ProcessScheduled(RHI::IDevice* device) {
            if (!device) {
                return false;
            }

            ResourceState currentState = m_State.load(std::memory_order_acquire);
            
            // Process scheduled create
            if (currentState == ResourceState::ScheduledCreate) {
                m_State.store(ResourceState::Creating, std::memory_order_release);
                
                // Set device (protected member, can be accessed directly in implementation)
                m_Device = device;
                
                bool success = DoCreate(device);
                if (success) {
                    m_State.store(ResourceState::Created, std::memory_order_release);
                } else {
                    m_State.store(ResourceState::Uninitialized, std::memory_order_release);
                    m_Device = nullptr;
                }
                return true;
            }
            
            // Process scheduled update
            if (currentState == ResourceState::ScheduledUpdate) {
                m_State.store(ResourceState::Updating, std::memory_order_release);
                
                bool success = DoUpdate(device);
                if (success) {
                    m_State.store(ResourceState::Created, std::memory_order_release);
                } else {
                    // Update failed, but resource might still be usable
                    m_State.store(ResourceState::Created, std::memory_order_release);
                }
                return true;
            }
            
            // Process scheduled destroy
            if (currentState == ResourceState::ScheduledDestroy) {
                m_State.store(ResourceState::Destroying, std::memory_order_release);
                
                DoDestroy();
                
                m_State.store(ResourceState::Destroyed, std::memory_order_release);
                m_Device = nullptr;
                return true;
            }
            
            return false; // No scheduled operation
        }


        void IRenderResource::SetState(ResourceState state) {
            m_State.store(state, std::memory_order_release);
        }

    } // namespace Renderer
} // namespace FirstEngine
