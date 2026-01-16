// Example: Multi-threaded Engine Usage
// This file demonstrates how to use the FirstEngine threading system

#include "FirstEngine/Core/ThreadManager.h"
#include "FirstEngine/Core/Barrier.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace FirstEngine::Core;

void ExampleBasicUsage() {
    std::cout << "=== Example 1: Basic Thread Invocation ===\n";

    // Initialize thread manager (creates all engine threads)
    ThreadManager::Initialize();
    ThreadManager& tm = ThreadManager::GetInstance();

    // Invoke a task on the Game thread
    auto future = tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "Task executed on Game thread\n";
    });

    // Wait for task to complete
    future.wait();
    std::cout << "Task completed\n\n";
}

void ExampleCrossThreadCommunication() {
    std::cout << "=== Example 2: Cross-Thread Communication ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Device thread waits for Render thread to complete FrameGraph
    auto renderFuture = tm.InvokeOnThread(ThreadType::Render, []() {
        std::cout << "[Render] Executing FrameGraph...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate work
        std::cout << "[Render] FrameGraph completed\n";
    }, TaskPriority::Critical);

    // Device thread waits for render to complete
    auto deviceFuture = tm.InvokeOnThread(ThreadType::Device, [&renderFuture]() {
        std::cout << "[Device] Waiting for Render thread...\n";
        renderFuture.wait(); // Wait for render to complete
        std::cout << "[Device] Render completed, continuing device operations\n";
    }, TaskPriority::High);

    deviceFuture.wait();
    std::cout << "Cross-thread communication completed\n\n";
}

void ExampleBarrierSynchronization() {
    std::cout << "=== Example 3: Barrier Synchronization ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Create a barrier for 3 threads
    auto barrier = tm.CreateBarrier(3);

    // All three threads will wait at the barrier
    auto future1 = tm.InvokeOnThread(ThreadType::Game, [barrier]() {
        std::cout << "[Game] Reached barrier\n";
        barrier->Wait();
        std::cout << "[Game] Passed barrier\n";
    });

    auto future2 = tm.InvokeOnThread(ThreadType::IO, [barrier]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[IO] Reached barrier\n";
        barrier->Wait();
        std::cout << "[IO] Passed barrier\n";
    });

    auto future3 = tm.InvokeOnThread(ThreadType::Python, [barrier]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[Python] Reached barrier\n";
        barrier->Wait();
        std::cout << "[Python] Passed barrier\n";
    });

    // Wait for all threads
    future1.wait();
    future2.wait();
    future3.wait();
    std::cout << "All threads passed barrier\n\n";
}

void ExampleTaskPriority() {
    std::cout << "=== Example 4: Task Priority ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Submit tasks with different priorities
    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "Low priority task\n";
    }, TaskPriority::Low);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "Normal priority task\n";
    }, TaskPriority::Normal);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "High priority task\n";
    }, TaskPriority::High);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "Critical priority task\n";
    }, TaskPriority::Critical);

    // Wait a bit for tasks to process
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Priority tasks completed (Critical should execute first)\n\n";
}

void ExampleReturnValues() {
    std::cout << "=== Example 5: Task Return Values ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Invoke task with return value
    auto future = tm.InvokeOnThread(ThreadType::Game, []() -> int {
        return 42;
    });

    int result = future.get();
    std::cout << "Task returned: " << result << "\n\n";
}

void ExampleDeviceRenderSync() {
    std::cout << "=== Example 6: Device-Render Synchronization ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Render thread executes FrameGraph
    auto renderFuture = tm.InvokeOnThread(ThreadType::Render, []() {
        std::cout << "[Render] Building FrameGraph...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[Render] FrameGraph built\n";
    }, TaskPriority::Critical);

    // Device thread waits for render, then processes
    auto deviceFuture = tm.InvokeOnThread(ThreadType::Device, [&renderFuture]() {
        std::cout << "[Device] Waiting for FrameGraph...\n";
        renderFuture.wait();
        std::cout << "[Device] FrameGraph ready, processing device operations\n";
    }, TaskPriority::High);

    deviceFuture.wait();
    std::cout << "Device-Render synchronization completed\n\n";
}

int main() {
    try {
        ExampleBasicUsage();
        ExampleCrossThreadCommunication();
        ExampleBarrierSynchronization();
        ExampleTaskPriority();
        ExampleReturnValues();
        ExampleDeviceRenderSync();

        // Shutdown thread manager
        ThreadManager::Shutdown();
        std::cout << "All examples completed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
