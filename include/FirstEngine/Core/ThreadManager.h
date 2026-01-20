#pragma once

#include "FirstEngine/Core/Export.h"
#include "FirstEngine/Core/Thread.h"
#include "FirstEngine/Core/Task.h"
#include "FirstEngine/Core/Barrier.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>

namespace FirstEngine {
    namespace Core {

        // ThreadManager - manages all engine threads
        class FE_CORE_API ThreadManager {
        public:
            static ThreadManager& GetInstance();
            static void Initialize();
            static void Shutdown();

            // Non-copyable
            ThreadManager(const ThreadManager&) = delete;
            ThreadManager& operator=(const ThreadManager&) = delete;

            // Create and register a thread
            Thread* CreateThread(ThreadType type, const std::string& name, ThreadPriority priority = ThreadPriority::Normal);

            // Get thread by type
            Thread* GetThread(ThreadType type);

            // Get thread by name
            Thread* GetThread(const std::string& name);

            // Invoke task on a specific thread
            Future<void> InvokeOnThread(ThreadType type, std::function<void()> task, TaskPriority priority = TaskPriority::Normal);

            // Invoke task on a specific thread with return value
            template<typename T>
            Future<T> InvokeOnThread(ThreadType type, std::function<T()> task, TaskPriority priority = TaskPriority::Normal);

            // Wait for all threads to finish their current tasks
            void WaitForAllThreads();

            // Wait for specific thread to finish its tasks
            void WaitForThread(ThreadType type);

            // Create a barrier for synchronization
            std::shared_ptr<Barrier> CreateBarrier(size_t count);

            // Shutdown all threads
            void ShutdownAll();

            // Get statistics
            struct ThreadStats {
                std::string name;
                ThreadType type;
                size_t pendingTasks;
                uint64_t tasksProcessed;
            };
            std::vector<ThreadStats> GetThreadStats() const;

        private:
            ThreadManager();
            ~ThreadManager();

            mutable std::mutex m_Mutex;
            std::unordered_map<ThreadType, std::unique_ptr<Thread>> m_Threads;
            std::unordered_map<std::string, Thread*> m_ThreadsByName;
            bool m_Initialized;
        };

        // Template implementation
        template<typename T>
        Future<T> ThreadManager::InvokeOnThread(ThreadType type, std::function<T()> task, TaskPriority priority) {
            Thread* thread = GetThread(type);
            if (!thread) {
                throw std::runtime_error("Thread not found: " + std::to_string(static_cast<int>(type)));
            }
            return thread->Invoke(task, priority);
        }

    } // namespace Core
} // namespace FirstEngine
