// FirstEngine 多线程系统使用示例
// 演示如何在游戏引擎中使用多线程系统

#include "FirstEngine/Core/Threading.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>

using namespace FirstEngine::Core;

// 示例1: 基本任务调用
void Example1_BasicInvoke() {
    std::cout << "\n=== 示例1: 基本任务调用 ===\n";

    ThreadManager::Initialize();
    ThreadManager& tm = ThreadManager::GetInstance();

    // 在 Game 线程上执行任务
    auto future = tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "[Game Thread] 执行游戏逻辑更新\n";
    });

    future.wait();
    std::cout << "任务完成\n";
}

// 示例2: Device 线程等待 Render 线程完成 FrameGraph
void Example2_DeviceRenderSync() {
    std::cout << "\n=== 示例2: Device-Render 同步 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // Render 线程执行 FrameGraph
    std::cout << "[Main] 提交 FrameGraph 任务到 Render 线程\n";
    auto renderFuture = tm.InvokeOnThread(ThreadType::Render, []() {
        std::cout << "[Render Thread] 开始构建 FrameGraph...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 模拟工作
        std::cout << "[Render Thread] FrameGraph 构建完成\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[Render Thread] FrameGraph 执行完成\n";
    }, TaskPriority::Critical);

    // Device 线程等待 Render 完成后再执行
    std::cout << "[Main] 提交 Device 任务，等待 Render 完成\n";
    auto deviceFuture = tm.InvokeOnThread(ThreadType::Device, [&renderFuture]() {
        std::cout << "[Device Thread] 等待 Render 线程完成 FrameGraph...\n";
        renderFuture.wait(); // 阻塞等待 Render 完成
        std::cout << "[Device Thread] Render 完成，开始处理 Device 操作\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        std::cout << "[Device Thread] Device 操作完成\n";
    }, TaskPriority::High);

    deviceFuture.wait();
    std::cout << "[Main] Device-Render 同步完成\n";
}

// 示例3: 使用 Barrier 同步多个线程
void Example3_BarrierSync() {
    std::cout << "\n=== 示例3: Barrier 同步 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // 创建3个线程的屏障
    auto barrier = tm.CreateBarrier(3);

    std::cout << "[Main] 创建 Barrier(3)，3个线程必须都到达才能继续\n";

    // 三个线程都到达屏障后才会继续
    auto future1 = tm.InvokeOnThread(ThreadType::Game, [barrier]() {
        std::cout << "[Game Thread] 执行游戏逻辑...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "[Game Thread] 到达 Barrier\n";
        barrier->Wait();
        std::cout << "[Game Thread] 通过 Barrier，继续执行\n";
    });

    auto future2 = tm.InvokeOnThread(ThreadType::IO, [barrier]() {
        std::cout << "[IO Thread] 执行 I/O 操作...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[IO Thread] 到达 Barrier\n";
        barrier->Wait();
        std::cout << "[IO Thread] 通过 Barrier，继续执行\n";
    });

    auto future3 = tm.InvokeOnThread(ThreadType::Python, [barrier]() {
        std::cout << "[Python Thread] 执行 Python 脚本...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        std::cout << "[Python Thread] 到达 Barrier\n";
        barrier->Wait();
        std::cout << "[Python Thread] 通过 Barrier，继续执行\n";
    });

    // 等待所有线程完成
    future1.wait();
    future2.wait();
    future3.wait();
    std::cout << "[Main] 所有线程都通过了 Barrier\n";
}

// 示例4: 任务优先级
void Example4_TaskPriority() {
    std::cout << "\n=== 示例4: 任务优先级 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    std::cout << "[Main] 提交不同优先级的任务到 Game 线程\n";

    // 按顺序提交，但应该按优先级执行
    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "[Game Thread] Low 优先级任务执行\n";
    }, TaskPriority::Low);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "[Game Thread] Normal 优先级任务执行\n";
    }, TaskPriority::Normal);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "[Game Thread] High 优先级任务执行\n";
    }, TaskPriority::High);

    tm.InvokeOnThread(ThreadType::Game, []() {
        std::cout << "[Game Thread] Critical 优先级任务执行\n";
    }, TaskPriority::Critical);

    // 等待任务执行
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "[Main] 注意：Critical 应该最先执行，Low 最后执行\n";
}

// 示例5: 获取任务返回值
void Example5_ReturnValues() {
    std::cout << "\n=== 示例5: 任务返回值 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // 执行带返回值的任务
    auto future = tm.InvokeOnThread(ThreadType::Game, []() -> int {
        std::cout << "[Game Thread] 计算中...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return 42;
    });

    int result = future.get(); // 阻塞直到任务完成
    std::cout << "[Main] 任务返回值: " << result << "\n";
}

// 示例6: 多线程资源加载
void Example6_ParallelLoading() {
    std::cout << "\n=== 示例6: 并行资源加载 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // IO 线程并行加载多个资源
    auto textureFuture = tm.InvokeOnThread(ThreadType::IO, []() -> std::string {
        std::cout << "[IO Thread] 加载纹理...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return "texture.png";
    });

    auto modelFuture = tm.InvokeOnThread(ThreadType::IO, []() -> std::string {
        std::cout << "[IO Thread] 加载模型...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return "model.fbx";
    });

    auto materialFuture = tm.InvokeOnThread(ThreadType::IO, []() -> std::string {
        std::cout << "[IO Thread] 加载材质...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
        return "material.mat";
    });

    // Game 线程等待所有资源加载完成
    tm.InvokeOnThread(ThreadType::Game, [&textureFuture, &modelFuture, &materialFuture]() {
        std::cout << "[Game Thread] 等待资源加载完成...\n";
        auto texture = textureFuture.get();
        auto model = modelFuture.get();
        auto material = materialFuture.get();
        std::cout << "[Game Thread] 所有资源加载完成:\n";
        std::cout << "  - Texture: " << texture << "\n";
        std::cout << "  - Model: " << model << "\n";
        std::cout << "  - Material: " << material << "\n";
    }).wait();

    std::cout << "[Main] 并行加载完成\n";
}

// 示例7: 帧同步（使用 Barrier）
void Example7_FrameSync() {
    std::cout << "\n=== 示例7: 帧同步 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();

    // 模拟一帧的同步
    auto frameBarrier = tm.CreateBarrier(4); // 4个线程需要同步

    std::cout << "[Main] 开始新的一帧，所有线程需要同步\n";

    tm.InvokeOnThread(ThreadType::Game, [frameBarrier]() {
        std::cout << "[Game] 更新游戏逻辑\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        frameBarrier->Wait();
        std::cout << "[Game] 帧同步完成\n";
    });

    tm.InvokeOnThread(ThreadType::Render, [frameBarrier]() {
        std::cout << "[Render] 渲染帧\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        frameBarrier->Wait();
        std::cout << "[Render] 帧同步完成\n";
    });

    tm.InvokeOnThread(ThreadType::Device, [frameBarrier]() {
        std::cout << "[Device] 处理设备操作\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        frameBarrier->Wait();
        std::cout << "[Device] 帧同步完成\n";
    });

    tm.InvokeOnThread(ThreadType::IO, [frameBarrier]() {
        std::cout << "[IO] 处理 I/O\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        frameBarrier->Wait();
        std::cout << "[IO] 帧同步完成\n";
    });

    // 等待所有线程完成
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "[Main] 帧同步完成，可以开始下一帧\n";
}

// 示例8: 使用 Event 进行信号通知
void Example8_EventSignal() {
    std::cout << "\n=== 示例8: Event 信号通知 ===\n";

    ThreadManager& tm = ThreadManager::GetInstance();
    Event event;

    // Game 线程等待事件
    auto gameFuture = tm.InvokeOnThread(ThreadType::Game, [&event]() {
        std::cout << "[Game Thread] 等待资源准备就绪...\n";
        event.Wait();
        std::cout << "[Game Thread] 资源已就绪，开始使用\n";
    });

    // IO 线程加载资源后设置事件
    auto ioFuture = tm.InvokeOnThread(ThreadType::IO, [&event]() {
        std::cout << "[IO Thread] 加载资源中...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "[IO Thread] 资源加载完成，通知 Game 线程\n";
        event.Set();
    });

    gameFuture.wait();
    ioFuture.wait();
    std::cout << "[Main] Event 通知完成\n";
}

int main() {
    try {
        std::cout << "FirstEngine 多线程系统示例\n";
        std::cout << "==========================\n";

        Example1_BasicInvoke();
        Example2_DeviceRenderSync();
        Example3_BarrierSync();
        Example4_TaskPriority();
        Example5_ReturnValues();
        Example6_ParallelLoading();
        Example7_FrameSync();
        Example8_EventSignal();

        // 显示线程统计
        ThreadManager& tm = ThreadManager::GetInstance();
        auto stats = tm.GetThreadStats();
        std::cout << "\n=== 线程统计 ===\n";
        for (const auto& stat : stats) {
            std::cout << stat.name << ": "
                      << stat.pendingTasks << " 待处理, "
                      << stat.tasksProcessed << " 已处理\n";
        }

        // 关闭线程管理器
        ThreadManager::Shutdown();
        std::cout << "\n所有示例完成！\n";
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
