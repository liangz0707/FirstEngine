#pragma once

#include "FirstEngine/Core/Export.h"
#include "FirstEngine/Core/Task.h"
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <exception>

namespace FirstEngine {
    namespace Core {

        // Forward declarations
        class ThreadManager;

        // Thread types in the engine
        enum class ThreadType {
            Device,     // Device/Vulkan thread
            Game,       // Game logic thread
            IO,         // I/O operations thread
            Python,     // Python scripting thread
            Render,     // Render thread
            Worker      // Generic worker thread
        };

        // Thread priority
        enum class ThreadPriority {
            Low,
            Normal,
            High,
            Critical
        };

        // Thread class - represents a single engine thread
        class FE_CORE_API Thread {
        public:
            Thread(ThreadType type, const std::string& name, ThreadPriority priority = ThreadPriority::Normal);
            ~Thread();

            // Non-copyable
            Thread(const Thread&) = delete;
            Thread& operator=(const Thread&) = delete;

            // Start the thread
            void Start();

            // Stop the thread (graceful shutdown)
            void Stop();

            // Wait for thread to finish
            void Join();

            // Invoke a task on this thread (thread-safe, can be called from any thread)
            Future Invoke(std::function<void()> task, TaskPriority priority = TaskPriority::Normal);

            // Invoke a task with return value
            template<typename T>
            Future<T> Invoke(std::function<T()> task, TaskPriority priority = TaskPriority::Normal);

            // Check if this is the current thread
            bool IsCurrentThread() const;

            // Get thread type
            ThreadType GetType() const { return m_Type; }

            // Get thread name
            const std::string& GetName() const { return m_Name; }

            // Get thread ID
            std::thread::id GetThreadId() const { return m_Thread ? m_Thread->get_id() : std::thread::id(); }

            // Wait for all pending tasks to complete
            void WaitForAllTasks();

            // Get number of pending tasks
            size_t GetPendingTaskCount() const;

            // Get number of processed tasks
            uint64_t GetProcessedTaskCount() const { return m_TasksProcessed.load(); }

        private:
            // Thread main loop
            void ThreadMain();

            // Process tasks from queue
            void ProcessTasks();

            ThreadType m_Type;
            std::string m_Name;
            ThreadPriority m_Priority;
            
            std::unique_ptr<std::thread> m_Thread;
            std::atomic<bool> m_ShouldStop;
            std::atomic<bool> m_IsRunning;

            // Task queue (priority queue)
            mutable std::mutex m_QueueMutex;
            std::condition_variable m_QueueCondition;
            std::priority_queue<Task, std::vector<Task>, TaskComparator> m_TaskQueue;

            // Statistics
            std::atomic<uint64_t> m_TasksProcessed;
            std::atomic<uint64_t> m_TasksPending;
        };

        // Template implementation
        template<typename T>
        Future<T> Thread::Invoke(std::function<T()> task, TaskPriority priority) {
            auto promise = std::make_shared<std::promise<T>>();
            Future<T> future = promise->get_future();

            Task taskWrapper([task, promise]() {
                try {
                    T result = task();
                    promise->set_value(result);
                } catch (...) {
                    promise->set_exception(std::current_exception());
                }
            }, priority);

            {
                std::lock_guard<std::mutex> lock(m_QueueMutex);
                m_TaskQueue.push(taskWrapper);
                m_TasksPending++;
            }

            m_QueueCondition.notify_one();
            return future;
        }

    } // namespace Core
} // namespace FirstEngine
