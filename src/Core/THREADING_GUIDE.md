# FirstEngine 多线程系统使用指南

## 概述

FirstEngine 的多线程系统提供了完整的线程管理、任务调度和同步机制，支持游戏引擎中常见的多线程场景。

## 核心组件

### 1. ThreadManager（线程管理器）

线程管理器是系统的核心，负责创建和管理所有引擎线程。

```cpp
// 初始化线程管理器（自动创建所有引擎线程）
ThreadManager::Initialize();

// 获取线程管理器实例
ThreadManager& tm = ThreadManager::GetInstance();

// 关闭所有线程
ThreadManager::Shutdown();
```

### 2. Thread（线程）

每个线程代表引擎中的一个专用线程，具有自己的任务队列。

**线程类型：**
- `ThreadType::Device` - Device/Vulkan 线程
- `ThreadType::Game` - 游戏逻辑线程
- `ThreadType::IO` - I/O 操作线程
- `ThreadType::Python` - Python 脚本线程
- `ThreadType::Render` - 渲染线程

### 3. Task（任务）

任务是在线程上执行的工作单元，支持优先级。

**任务优先级：**
- `TaskPriority::Low` - 低优先级
- `TaskPriority::Normal` - 普通优先级
- `TaskPriority::High` - 高优先级
- `TaskPriority::Critical` - 关键优先级

### 4. Future（未来对象）

Future 用于等待任务完成并获取返回值。

### 5. Barrier（屏障）

Barrier 用于多个线程的同步点，所有线程必须到达屏障才能继续。

### 6. Event（事件）

Event 用于线程间的信号通知。

## 基本用法

### 在任何线程上执行任务

```cpp
ThreadManager& tm = ThreadManager::GetInstance();

// 在 Game 线程上执行任务
auto future = tm.InvokeOnThread(ThreadType::Game, []() {
    // 任务代码
    std::cout << "Executing on Game thread\n";
});

// 等待任务完成
future.wait();
```

### 获取任务返回值

```cpp
// 执行带返回值的任务
auto future = tm.InvokeOnThread(ThreadType::Game, []() -> int {
    return 42;
});

int result = future.get(); // 阻塞直到任务完成并返回结果
```

### 线程间同步

```cpp
// Render 线程执行 FrameGraph
auto renderFuture = tm.InvokeOnThread(ThreadType::Render, []() {
    // 执行 FrameGraph
}, TaskPriority::Critical);

// Device 线程等待 Render 完成
auto deviceFuture = tm.InvokeOnThread(ThreadType::Device, [&renderFuture]() {
    renderFuture.wait(); // 等待 Render 线程完成
    // 继续 Device 操作
}, TaskPriority::High);
```

### 使用 Barrier 同步多个线程

```cpp
auto barrier = tm.CreateBarrier(3); // 3个线程的屏障

// 三个线程都到达屏障后才会继续
tm.InvokeOnThread(ThreadType::Game, [barrier]() {
    barrier->Wait();
});

tm.InvokeOnThread(ThreadType::IO, [barrier]() {
    barrier->Wait();
});

tm.InvokeOnThread(ThreadType::Python, [barrier]() {
    barrier->Wait();
});
```

### 使用 Event 进行信号通知

```cpp
Event event;

// 线程1：等待事件
tm.InvokeOnThread(ThreadType::Game, [&event]() {
    event.Wait(); // 等待事件被设置
    // 继续执行
});

// 线程2：设置事件
tm.InvokeOnThread(ThreadType::IO, [&event]() {
    // 执行一些操作
    event.Set(); // 唤醒等待的线程
});
```

## 线程执行流程图

```
┌─────────────────────────────────────────────────────────────┐
│                    Main Thread                              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadManager::Initialize()                         │  │
│  │    ├─ Create DeviceThread                            │  │
│  │    ├─ Create GameThread                              │  │
│  │    ├─ Create IOThread                                │  │
│  │    ├─ Create PythonThread                            │  │
│  │    └─ Create RenderThread                            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Device Thread (High Priority)                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadMain()                                        │  │
│  │    ├─ ProcessTasks()                                 │  │
│  │    │   ├─ Wait for tasks in queue                   │  │
│  │    │   ├─ Execute task (priority order)             │  │
│  │    │   └─ Update statistics                         │  │
│  │    └─ Loop until Stop()                             │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Game Thread (Normal Priority)                  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadMain()                                        │  │
│  │    ├─ ProcessTasks()                                 │  │
│  │    └─ Game logic updates                             │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Render Thread (Critical Priority)              │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadMain()                                        │  │
│  │    ├─ ProcessTasks()                                 │  │
│  │    ├─ Execute FrameGraph                             │  │
│  │    └─ Signal completion to Device thread             │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              IO Thread (Low Priority)                       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadMain()                                        │  │
│  │    ├─ ProcessTasks()                                 │  │
│  │    └─ File I/O operations                            │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────┐
│              Python Thread (Normal Priority)                │
│  ┌──────────────────────────────────────────────────────┐  │
│  │  ThreadMain()                                        │  │
│  │    ├─ ProcessTasks()                                 │  │
│  │    └─ Python script execution                        │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 典型使用场景

### 场景1：Device 线程等待 Render 线程

```cpp
// Render 线程执行 FrameGraph
auto renderTask = tm.InvokeOnThread(ThreadType::Render, []() {
    // 执行 FrameGraph 构建和渲染
    BuildFrameGraph();
    ExecuteFrameGraph();
}, TaskPriority::Critical);

// Device 线程等待 Render 完成后再继续
tm.InvokeOnThread(ThreadType::Device, [&renderTask]() {
    renderTask.wait(); // 等待 FrameGraph 完成
    // 现在可以安全地进行 Device 操作
    ProcessDeviceOperations();
}, TaskPriority::High);
```

### 场景2：多线程资源加载

```cpp
// IO 线程加载资源
auto textureFuture = tm.InvokeOnThread(ThreadType::IO, []() {
    return LoadTexture("texture.png");
});

auto modelFuture = tm.InvokeOnThread(ThreadType::IO, []() {
    return LoadModel("model.fbx");
});

// Game 线程等待资源加载完成
tm.InvokeOnThread(ThreadType::Game, [&textureFuture, &modelFuture]() {
    auto texture = textureFuture.get();
    auto model = modelFuture.get();
    // 使用加载的资源
    UseResources(texture, model);
});
```

### 场景3：帧同步

```cpp
// 使用 Barrier 同步所有线程完成一帧
auto frameBarrier = tm.CreateBarrier(5); // 5个线程

tm.InvokeOnThread(ThreadType::Game, [frameBarrier]() {
    UpdateGameLogic();
    frameBarrier->Wait();
});

tm.InvokeOnThread(ThreadType::Render, [frameBarrier]() {
    RenderFrame();
    frameBarrier->Wait();
});

// ... 其他线程也等待屏障

// 所有线程都到达屏障后，开始下一帧
```

## 最佳实践

1. **任务粒度**：保持任务小而专注（1-10微秒执行时间）
2. **优先级使用**：合理使用任务优先级，关键路径使用 Critical
3. **避免阻塞**：避免在任务中执行长时间阻塞操作
4. **错误处理**：任务中的异常会被捕获，不会崩溃线程
5. **资源管理**：确保共享资源有适当的同步机制

## 性能考虑

- 任务队列使用优先级队列，高优先级任务优先执行
- 使用条件变量实现高效的任务等待
- 支持任务窃取（未来可扩展）
- 线程优先级根据 ThreadPriority 设置

## 调试和监控

```cpp
// 获取线程统计信息
auto stats = tm.GetThreadStats();
for (const auto& stat : stats) {
    std::cout << stat.name << ": "
              << stat.pendingTasks << " pending, "
              << stat.tasksProcessed << " processed\n";
}
```
