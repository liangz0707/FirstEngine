# FirstEngine 多线程系统

## 概述

FirstEngine 多线程系统提供了完整的线程管理、任务调度和同步机制，专为游戏引擎设计。系统支持多个专用线程，线程间可以安全地传递任务和同步。

## 核心特性

- ✅ **多线程支持**: Device、Game、IO、Python、Render 五个专用线程
- ✅ **任务队列**: 每个线程拥有独立的任务队列，支持优先级
- ✅ **跨线程调用**: 可以从任何线程向其他线程提交任务
- ✅ **同步机制**: Future、Barrier、Event 等多种同步原语
- ✅ **线程优先级**: 支持设置线程优先级（Low、Normal、High、Critical）
- ✅ **任务优先级**: 支持任务优先级，高优先级任务优先执行
- ✅ **异常安全**: 任务中的异常不会崩溃线程

## 快速开始

### 1. 初始化线程系统

```cpp
#include "FirstEngine/Core/Threading.h"

// 初始化线程管理器（自动创建所有引擎线程）
ThreadManager::Initialize();

// 获取线程管理器实例
ThreadManager& tm = ThreadManager::GetInstance();
```

### 2. 在任何线程上执行任务

```cpp
// 在 Game 线程上执行任务
auto future = tm.InvokeOnThread(ThreadType::Game, []() {
    // 游戏逻辑更新
    UpdateGameLogic();
});

// 等待任务完成
future.wait();
```

### 3. Device 线程等待 Render 线程

```cpp
// Render 线程执行 FrameGraph
auto renderFuture = tm.InvokeOnThread(ThreadType::Render, []() {
    BuildFrameGraph();
    ExecuteFrameGraph();
}, TaskPriority::Critical);

// Device 线程等待 Render 完成
tm.InvokeOnThread(ThreadType::Device, [&renderFuture]() {
    renderFuture.wait(); // 等待 FrameGraph 完成
    ProcessDeviceOperations();
}, TaskPriority::High).wait();
```

### 4. 关闭线程系统

```cpp
// 关闭所有线程
ThreadManager::Shutdown();
```

## API 参考

### ThreadManager

**主要方法：**
- `Initialize()` - 初始化线程管理器
- `GetInstance()` - 获取单例实例
- `InvokeOnThread()` - 在指定线程上执行任务
- `GetThread()` - 获取线程对象
- `CreateBarrier()` - 创建同步屏障
- `WaitForThread()` - 等待线程完成所有任务
- `Shutdown()` - 关闭所有线程

### Thread

**线程类型：**
- `ThreadType::Device` - Device/Vulkan 线程（高优先级）
- `ThreadType::Game` - 游戏逻辑线程（普通优先级）
- `ThreadType::IO` - I/O 操作线程（低优先级）
- `ThreadType::Python` - Python 脚本线程（普通优先级）
- `ThreadType::Render` - 渲染线程（关键优先级）

### TaskPriority

**任务优先级：**
- `TaskPriority::Low` - 低优先级
- `TaskPriority::Normal` - 普通优先级
- `TaskPriority::High` - 高优先级
- `TaskPriority::Critical` - 关键优先级

### Barrier

用于多个线程的同步点。

```cpp
auto barrier = tm.CreateBarrier(3); // 3个线程的屏障

// 所有3个线程都到达屏障后才会继续
thread1->Invoke([barrier]() { barrier->Wait(); });
thread2->Invoke([barrier]() { barrier->Wait(); });
thread3->Invoke([barrier]() { barrier->Wait(); });
```

### Event

用于线程间的信号通知。

```cpp
Event event;

// 线程1等待事件
thread1->Invoke([&event]() { event.Wait(); });

// 线程2设置事件
thread2->Invoke([&event]() { event.Set(); });
```

## 设计原则

1. **线程专用性**: 每个线程有明确的职责
2. **任务粒度**: 任务应该小而专注（1-10微秒）
3. **优先级管理**: 关键路径使用 Critical 优先级
4. **异常安全**: 任务异常不会影响线程
5. **无锁设计**: 尽可能减少锁竞争

## 性能特性

- 优先级队列确保关键任务优先执行
- 条件变量实现高效的任务等待
- 线程优先级根据 ThreadPriority 设置
- 支持任务返回值（Future 模式）

## 示例代码

完整示例请参考：
- `src/Core/ThreadingExample.cpp` - 完整的使用示例
- `src/Core/THREADING_GUIDE.md` - 详细使用指南
- `src/Core/THREADING_FLOWCHART.md` - 执行流程图
