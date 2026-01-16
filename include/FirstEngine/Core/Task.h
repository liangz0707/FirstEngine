#pragma once

#include "FirstEngine/Core/Export.h"
#include <functional>
#include <memory>
#include <future>
#include <atomic>
#include <chrono>
#include <exception>

namespace FirstEngine {
    namespace Core {

        // Task priority
        enum class TaskPriority : int {
            Low = 0,
            Normal = 1,
            High = 2,
            Critical = 3
        };

        // Task class - represents a unit of work
        class FE_CORE_API Task {
        public:
            Task() : m_Function(nullptr), m_Priority(TaskPriority::Normal), m_TaskId(0) {}
            
            Task(std::function<void()> func, TaskPriority priority = TaskPriority::Normal)
                : m_Function(std::move(func)), m_Priority(priority), m_TaskId(GetNextTaskId()) {}

            void operator()() const {
                if (m_Function) {
                    m_Function();
                }
            }

            TaskPriority GetPriority() const { return m_Priority; }
            uint64_t GetTaskId() const { return m_TaskId; }

            bool operator<(const Task& other) const {
                // Higher priority tasks come first
                if (m_Priority != other.m_Priority) {
                    return static_cast<int>(m_Priority) < static_cast<int>(other.m_Priority);
                }
                // If same priority, earlier tasks come first
                return m_TaskId > other.m_TaskId;
            }

        private:
            std::function<void()> m_Function;
            TaskPriority m_Priority;
            uint64_t m_TaskId;

            static uint64_t GetNextTaskId() {
                static std::atomic<uint64_t> s_NextTaskId(1);
                return s_NextTaskId++;
            }
        };

        // Task comparator for priority queue
        struct TaskComparator {
            bool operator()(const Task& a, const Task& b) const {
                return a < b;
            }
        };

        // Future wrapper for task results
        template<typename T = void>
        using Future = std::future<T>;

        // Promise wrapper for task results
        template<typename T = void>
        using Promise = std::promise<T>;

    } // namespace Core
} // namespace FirstEngine
