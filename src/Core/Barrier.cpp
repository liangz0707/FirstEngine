#include "FirstEngine/Core/Barrier.h"
#include <stdexcept>
#include <chrono>

namespace FirstEngine {
    namespace Core {

        Barrier::Barrier(size_t count) : m_Count(count), m_InitialCount(count), m_Generation(0) {
            if (count == 0) {
                throw std::invalid_argument("Barrier count must be greater than 0");
            }
        }

        Barrier::~Barrier() = default;

        void Barrier::Wait() {
            std::unique_lock<std::mutex> lock(m_Mutex);
            size_t gen = m_Generation;
            size_t count = m_Count;

            if (--m_Count == 0) {
                // Last thread to reach barrier
                m_Generation++;
                m_Count = m_InitialCount; // Reset count for next barrier
                m_Condition.notify_all();
            } else {
                // Wait for other threads
                m_Condition.wait(lock, [this, gen] {
                    return gen != m_Generation;
                });
            }
        }

        bool Barrier::WaitFor(std::chrono::milliseconds timeout) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            size_t gen = m_Generation;

            if (--m_Count == 0) {
                // Last thread to reach barrier
                m_Generation++;
                m_Count = m_InitialCount; // Reset count for next barrier
                m_Condition.notify_all();
                return true;
            } else {
                // Wait for other threads with timeout
                bool result = m_Condition.wait_for(lock, timeout, [this, gen] {
                    return gen != m_Generation;
                });
                return result;
            }
        }

        void Barrier::Reset(size_t count) {
            if (count == 0) {
                throw std::invalid_argument("Barrier count must be greater than 0");
            }

            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Count = count;
            m_InitialCount = count;
            m_Generation = 0;
        }

        Event::Event(bool initialState) : m_Signaled(initialState) {
        }

        void Event::Set() {
            {
                std::lock_guard<std::mutex> lock(m_Mutex);
                m_Signaled = true;
            }
            m_Condition.notify_all();
        }

        void Event::Reset() {
            std::lock_guard<std::mutex> lock(m_Mutex);
            m_Signaled = false;
        }

        void Event::Wait() {
            std::unique_lock<std::mutex> lock(m_Mutex);
            m_Condition.wait(lock, [this] {
                return m_Signaled.load();
            });
        }

        bool Event::WaitFor(std::chrono::milliseconds timeout) {
            std::unique_lock<std::mutex> lock(m_Mutex);
            return m_Condition.wait_for(lock, timeout, [this] {
                return m_Signaled.load();
            });
        }

        bool Event::IsSet() const {
            return m_Signaled.load();
        }

    } // namespace Core
} // namespace FirstEngine
