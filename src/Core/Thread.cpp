#include "FirstEngine/Core/Thread.h"
#include "FirstEngine/Core/ThreadManager.h"
#include <iostream>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#include <sched.h>
#endif

namespace FirstEngine {
    namespace Core {

        Thread::Thread(ThreadType type, const std::string& name, ThreadPriority priority)
            : m_Type(type)
            , m_Name(name)
            , m_Priority(priority)
            , m_ShouldStop(false)
            , m_IsRunning(false)
            , m_TasksProcessed(0)
            , m_TasksPending(0) {
        }

        Thread::~Thread() {
            Stop();
            Join();
        }

        void Thread::Start() {
            if (m_IsRunning) {
                return;
            }

            m_ShouldStop = false;
            m_IsRunning = true;
            m_Thread = std::make_unique<std::thread>(&Thread::ThreadMain, this);

            // Set thread priority based on ThreadPriority
#ifdef _WIN32
            HANDLE handle = m_Thread->native_handle();
            int priorityLevel = THREAD_PRIORITY_NORMAL;
            switch (m_Priority) {
                case ThreadPriority::Low:
                    priorityLevel = THREAD_PRIORITY_BELOW_NORMAL;
                    break;
                case ThreadPriority::Normal:
                    priorityLevel = THREAD_PRIORITY_NORMAL;
                    break;
                case ThreadPriority::High:
                    priorityLevel = THREAD_PRIORITY_ABOVE_NORMAL;
                    break;
                case ThreadPriority::Critical:
                    priorityLevel = THREAD_PRIORITY_HIGHEST;
                    break;
            }
            SetThreadPriority(handle, priorityLevel);
#else
            pthread_t handle = m_Thread->native_handle();
            int policy;
            struct sched_param param;
            pthread_getschedparam(handle, &policy, &param);
            switch (m_Priority) {
                case ThreadPriority::Low:
                    param.sched_priority = sched_get_priority_min(policy);
                    break;
                case ThreadPriority::Normal:
                    param.sched_priority = (sched_get_priority_min(policy) + sched_get_priority_max(policy)) / 2;
                    break;
                case ThreadPriority::High:
                    param.sched_priority = sched_get_priority_max(policy) - 1;
                    break;
                case ThreadPriority::Critical:
                    param.sched_priority = sched_get_priority_max(policy);
                    break;
            }
            pthread_setschedparam(handle, policy, &param);
#endif
        }

        void Thread::Stop() {
            if (!m_IsRunning) {
                return;
            }

            m_ShouldStop = true;
            m_QueueCondition.notify_all();
        }

        void Thread::Join() {
            if (m_Thread && m_Thread->joinable()) {
                m_Thread->join();
            }
        }

        Future<void> Thread::Invoke(std::function<void()> task, TaskPriority priority) {
            auto promise = std::make_shared<std::promise<void>>();
            Future<void> future = promise->get_future();

            Task taskWrapper([task, promise]() {
                try {
                    task();
                    promise->set_value();
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

        bool Thread::IsCurrentThread() const {
            if (!m_Thread) {
                return false;
            }
            return std::this_thread::get_id() == m_Thread->get_id();
        }

        void Thread::WaitForAllTasks() {
            while (true) {
                {
                    std::lock_guard<std::mutex> lock(m_QueueMutex);
                    if (m_TaskQueue.empty() && m_TasksPending == 0) {
                        break;
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }

        size_t Thread::GetPendingTaskCount() const {
            std::lock_guard<std::mutex> lock(m_QueueMutex);
            return m_TaskQueue.size();
        }

        void Thread::ThreadMain() {
            // Set thread name (platform-specific)
#ifdef _WIN32
            // Windows: SetThreadDescription (Windows 10 version 1607+)
            typedef HRESULT(WINAPI* SetThreadDescriptionFunc)(HANDLE, PCWSTR);
            HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
            if (kernel32) {
                SetThreadDescriptionFunc setThreadDesc = (SetThreadDescriptionFunc)GetProcAddress(kernel32, "SetThreadDescription");
                if (setThreadDesc) {
                    std::wstring wname(m_Name.begin(), m_Name.end());
                    setThreadDesc(GetCurrentThread(), wname.c_str());
                }
            }
#else
            // Linux/Unix: pthread_setname_np
            pthread_setname_np(pthread_self(), m_Name.c_str());
#endif

            while (!m_ShouldStop || !m_TaskQueue.empty()) {
                ProcessTasks();
            }

            m_IsRunning = false;
        }

        void Thread::ProcessTasks() {
            std::unique_lock<std::mutex> lock(m_QueueMutex);

            // Wait for tasks or stop signal
            m_QueueCondition.wait(lock, [this] {
                return !m_TaskQueue.empty() || m_ShouldStop;
            });

            // Process all available tasks
            while (!m_TaskQueue.empty()) {
                Task task = m_TaskQueue.top();
                m_TaskQueue.pop();
                m_TasksPending--;

                lock.unlock();
                
                // Execute task
                try {
                    task();
                    m_TasksProcessed++;
                } catch (const std::exception& e) {
                    std::cerr << "Exception in thread " << m_Name << ": " << e.what() << std::endl;
                } catch (...) {
                    std::cerr << "Unknown exception in thread " << m_Name << std::endl;
                }

                lock.lock();
            }
        }

    } // namespace Core
} // namespace FirstEngine
