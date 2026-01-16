#include "FirstEngine/Core/ThreadManager.h"
#include <stdexcept>
#include <iostream>

namespace FirstEngine {
    namespace Core {

        static ThreadManager* s_Instance = nullptr;

        ThreadManager& ThreadManager::GetInstance() {
            if (!s_Instance) {
                throw std::runtime_error("ThreadManager not initialized. Call ThreadManager::Initialize() first.");
            }
            return *s_Instance;
        }

        void ThreadManager::Initialize() {
            if (s_Instance) {
                return; // Already initialized
            }
            s_Instance = new ThreadManager();
        }

        void ThreadManager::Shutdown() {
            if (s_Instance) {
                s_Instance->ShutdownAll();
                delete s_Instance;
                s_Instance = nullptr;
            }
        }

        ThreadManager::ThreadManager() : m_Initialized(false) {
            // Create default engine threads
            CreateThread(ThreadType::Device, "DeviceThread", ThreadPriority::High);
            CreateThread(ThreadType::Game, "GameThread", ThreadPriority::Normal);
            CreateThread(ThreadType::IO, "IOThread", ThreadPriority::Low);
            CreateThread(ThreadType::Python, "PythonThread", ThreadPriority::Normal);
            CreateThread(ThreadType::Render, "RenderThread", ThreadPriority::Critical);

            // Start all threads
            for (auto& pair : m_Threads) {
                pair.second->Start();
            }

            m_Initialized = true;
        }

        ThreadManager::~ThreadManager() {
            ShutdownAll();
        }

        Thread* ThreadManager::CreateThread(ThreadType type, const std::string& name, ThreadPriority priority) {
            std::lock_guard<std::mutex> lock(m_Mutex);

            // Check if thread already exists
            auto it = m_Threads.find(type);
            if (it != m_Threads.end()) {
                return it->second.get();
            }

            // Create new thread
            auto thread = std::make_unique<Thread>(type, name, priority);
            Thread* threadPtr = thread.get();
            m_Threads[type] = std::move(thread);
            m_ThreadsByName[name] = threadPtr;

            return threadPtr;
        }

        Thread* ThreadManager::GetThread(ThreadType type) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_Threads.find(type);
            return (it != m_Threads.end()) ? it->second.get() : nullptr;
        }

        Thread* ThreadManager::GetThread(const std::string& name) {
            std::lock_guard<std::mutex> lock(m_Mutex);
            auto it = m_ThreadsByName.find(name);
            return (it != m_ThreadsByName.end()) ? it->second : nullptr;
        }

        Future ThreadManager::InvokeOnThread(ThreadType type, std::function<void()> task, TaskPriority priority) {
            Thread* thread = GetThread(type);
            if (!thread) {
                throw std::runtime_error("Thread not found: " + std::to_string(static_cast<int>(type)));
            }
            return thread->Invoke(task, priority);
        }

        void ThreadManager::WaitForAllThreads() {
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (auto& pair : m_Threads) {
                pair.second->WaitForAllTasks();
            }
        }

        void ThreadManager::WaitForThread(ThreadType type) {
            Thread* thread = GetThread(type);
            if (thread) {
                thread->WaitForAllTasks();
            }
        }

        std::shared_ptr<Barrier> ThreadManager::CreateBarrier(size_t count) {
            return std::make_shared<Barrier>(count);
        }

        void ThreadManager::ShutdownAll() {
            std::lock_guard<std::mutex> lock(m_Mutex);
            for (auto& pair : m_Threads) {
                pair.second->Stop();
            }
            for (auto& pair : m_Threads) {
                pair.second->Join();
            }
            m_Threads.clear();
            m_ThreadsByName.clear();
            m_Initialized = false;
        }

        std::vector<ThreadManager::ThreadStats> ThreadManager::GetThreadStats() const {
            std::lock_guard<std::mutex> lock(m_Mutex);
            std::vector<ThreadStats> stats;
            stats.reserve(m_Threads.size());

            for (const auto& pair : m_Threads) {
                ThreadStats stat;
                stat.name = pair.second->GetName();
                stat.type = pair.second->GetType();
                stat.pendingTasks = pair.second->GetPendingTaskCount();
                stat.tasksProcessed = pair.second->GetProcessedTaskCount();
                stats.push_back(stat);
            }

            return stats;
        }

    } // namespace Core
} // namespace FirstEngine
