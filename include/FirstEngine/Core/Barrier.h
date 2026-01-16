#pragma once

#include "FirstEngine/Core/Export.h"
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace FirstEngine {
    namespace Core {

        // Barrier - synchronization primitive for multiple threads
        // Allows N threads to wait until all N threads reach the barrier
        class FE_CORE_API Barrier {
        public:
            explicit Barrier(size_t count);
            ~Barrier();

            // Wait at the barrier (blocks until all threads reach it)
            void Wait();

            // Try to wait with timeout (returns true if barrier was reached, false on timeout)
            bool WaitFor(std::chrono::milliseconds timeout);

            // Reset the barrier with a new count
            void Reset(size_t count);

            // Get current count
            size_t GetCount() const { 
                std::lock_guard<std::mutex> lock(m_Mutex);
                return m_Count; 
            }

        private:
            mutable std::mutex m_Mutex;
            size_t m_Count;
            size_t m_InitialCount;  // Store initial count for reset
            size_t m_Generation;
            std::condition_variable m_Condition;
        };

        // Event - synchronization primitive for signaling between threads
        class FE_CORE_API Event {
        public:
            Event(bool initialState = false);

            // Set the event (wake up all waiting threads)
            void Set();

            // Reset the event
            void Reset();

            // Wait for the event to be set
            void Wait();

            // Try to wait with timeout
            bool WaitFor(std::chrono::milliseconds timeout);

            // Check if event is set (non-blocking)
            bool IsSet() const;

        private:
            std::atomic<bool> m_Signaled;
            std::mutex m_Mutex;
            std::condition_variable m_Condition;
        };

    } // namespace Core
} // namespace FirstEngine
