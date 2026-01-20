# 架构重构 - RenderContext 统一管理渲染引擎状态

## 概述

本次重构将 `RenderEngine` 结构的功能迁移到 `RenderContext` 中，简化了 EditorAPI 接口，减少了 C# 需要暴露的渲染相关参数。

## 重构目标

1. **消除 RenderEngine 结构**：不再需要单独的 RenderEngine 结构，所有状态由 RenderContext 管理
2. **简化 C# 接口**：C# 不再需要持有 engine handle，只需要 viewport handle
3. **统一状态管理**：所有渲染引擎状态（device, pipeline, frameGraph, scene 等）统一由 RenderContext 管理
4. **减少参数暴露**：C# 只需要暴露必要的接口，内部细节由 RenderContext 处理

## 架构变化

### 重构前

```
C# RenderEngineService
  └─> 持有 _engineHandle (RenderEngine*)
      └─> RenderEngine 结构
          ├─> device, pipeline, frameGraph
          ├─> scene, renderConfig
          ├─> 同步对象
          └─> RenderContext (部分功能)

EditorAPI
  └─> 需要 RenderEngine* 参数
      └─> 访问 RenderEngine 的各个成员
```

### 重构后

```
C# RenderEngineService
  └─> 持有 _globalEngineHandle (静态，单例)
      └─> 实际是 RenderContext*

EditorAPI
  └─> 使用全局 RenderContext (g_GlobalRenderContext)
      └─> 所有状态由 RenderContext 内部管理

RenderContext (扩展)
  └─> 管理完整的渲染引擎状态
      ├─> device, pipeline, frameGraph (内部管理)
      ├─> scene, renderConfig (内部管理)
      ├─> 同步对象 (内部管理)
      └─> 命令缓冲区 (内部管理)
```

## 主要变更

### 1. RenderContext 扩展

**新增功能**：
- `InitializeEngine()` - 初始化完整的渲染引擎
- `ShutdownEngine()` - 关闭渲染引擎
- `LoadScene()` / `UnloadScene()` - 场景管理
- `SetRenderConfig()` - 渲染配置
- 内部管理所有渲染状态（device, pipeline, frameGraph, scene 等）

**内部状态**：
```cpp
class RenderContext {
private:
    // 渲染引擎状态（内部管理）
    bool m_EngineInitialized = false;
    RHI::IDevice* m_Device = nullptr;
    IRenderPipeline* m_RenderPipeline = nullptr;
    FrameGraph* m_FrameGraph = nullptr;
    Scene* m_Scene = nullptr;
    RenderConfig m_RenderConfig;
    
    // 同步对象（内部管理）
    RHI::FenceHandle m_InFlightFence = nullptr;
    RHI::SemaphoreHandle m_ImageAvailableSemaphore = nullptr;
    RHI::SemaphoreHandle m_RenderFinishedSemaphore = nullptr;
    
    // 命令缓冲区和记录器（内部管理）
    std::unique_ptr<RHI::ICommandBuffer> m_CommandBuffer;
    CommandRecorder m_CommandRecorder;
    uint32_t m_CurrentImageIndex = 0;
    
    // ...
};
```

### 2. EditorAPI 简化

**变化**：
- 移除 `RenderEngine` 结构定义
- 使用全局 `RenderContext*` (g_GlobalRenderContext)
- 所有函数参数从 `RenderEngine*` 改为 `void*`（实际是 RenderContext*）
- `RenderViewport` 不再需要 `engine` 指针

**新的接口**：
```cpp
// 创建引擎（返回 RenderContext*）
void* EditorAPI_CreateEngine(void* windowHandle, int width, int height);

// 初始化引擎
bool EditorAPI_InitializeEngine(void* engine);

// 渲染方法（使用全局 RenderContext）
void EditorAPI_PrepareFrameGraph(void* engine);
void EditorAPI_CreateResources(void* engine);
void EditorAPI_Render(void* engine);
void EditorAPI_SubmitFrame(RenderViewport* viewport);  // 不需要 engine 参数
```

### 3. C# RenderEngineService 简化

**变化**：
- 移除 `_engineHandle` 成员变量
- 使用静态 `_globalEngineHandle`（单例模式）
- 所有方法使用全局引擎句柄
- 添加 `ShutdownGlobalEngine()` 静态方法用于应用程序退出时清理

**新的实现**：
```csharp
public class RenderEngineService {
    private static IntPtr? _globalEngineHandle = null;  // 全局引擎句柄（单例）
    private IntPtr _viewportHandle;
    
    public bool Initialize(IntPtr windowHandle, int width, int height) {
        // 使用全局引擎句柄（单例）
        if (!_globalEngineHandle.HasValue || _globalEngineHandle.Value == IntPtr.Zero) {
            _globalEngineHandle = EditorAPI_CreateEngine(windowHandle, width, height);
            // ...
        }
    }
    
    // 静态方法：销毁全局引擎
    public static void ShutdownGlobalEngine() {
        if (_globalEngineHandle.HasValue && _globalEngineHandle.Value != IntPtr.Zero) {
            EditorAPI_ShutdownEngine(_globalEngineHandle.Value);
            EditorAPI_DestroyEngine(_globalEngineHandle.Value);
            _globalEngineHandle = null;
        }
    }
}
```

## 优势

1. **简化接口**：C# 不需要管理 engine handle，只需要 viewport handle
2. **统一管理**：所有渲染状态由 RenderContext 统一管理
3. **减少参数**：EditorAPI 函数参数更少，C# 暴露的接口更简洁
4. **易于维护**：渲染引擎状态集中在一个地方（RenderContext）
5. **线程安全**：使用 mutex 保护全局 RenderContext 访问

## 使用方式

### C# 使用

```csharp
// 初始化（自动创建全局引擎）
var renderEngine = new RenderEngineService();
renderEngine.Initialize(windowHandle, width, height);

// 创建视口
renderEngine.CreateViewport(parentHandle, x, y, width, height);

// 渲染循环
renderEngine.PrepareFrameGraph();
renderEngine.CreateResources();
renderEngine.Render();
renderEngine.SubmitFrame();

// 应用程序退出时
RenderEngineService.ShutdownGlobalEngine();
```

### C++ EditorAPI 使用

```cpp
// 创建全局引擎（单例）
void* engine = EditorAPI_CreateEngine(windowHandle, width, height);
EditorAPI_InitializeEngine(engine);

// 创建视口
RenderViewport* viewport = EditorAPI_CreateViewport(engine, parentHandle, x, y, w, h);

// 渲染循环
EditorAPI_PrepareFrameGraph(engine);
EditorAPI_CreateResources(engine);
EditorAPI_Render(engine);
EditorAPI_SubmitFrame(viewport);  // 不需要 engine 参数，使用全局 RenderContext
```

## 文件变更

### 修改的文件

- `include/FirstEngine/Renderer/RenderContext.h` - 扩展为管理完整渲染引擎状态
- `src/Renderer/RenderContext.cpp` - 实现引擎初始化、场景管理等功能
- `include/FirstEngine/Editor/EditorAPI.h` - 简化接口，移除 RenderEngine* 参数
- `src/Editor/EditorAPI.cpp` - 使用全局 RenderContext，移除 RenderEngine 结构
- `Editor/Services/RenderEngineService.cs` - 使用全局引擎句柄，简化接口

### 新增功能

- RenderContext 现在可以独立管理完整的渲染引擎
- 全局 RenderContext 单例模式（通过 EditorAPI 管理）
- C# 不再需要持有 engine handle

## 注意事项

1. **线程安全**：全局 RenderContext 使用 mutex 保护，确保多线程安全
2. **生命周期**：全局引擎在应用程序退出时应该调用 `ShutdownGlobalEngine()`
3. **向后兼容**：保留了旧的函数签名（标记为已废弃），但内部使用新实现

## 后续改进

1. **进一步简化**：可以考虑完全移除 engine 参数，只通过 viewport 操作
2. **多视口支持**：如果需要支持多个视口，可能需要调整架构
3. **错误处理**：增强错误处理和日志记录
