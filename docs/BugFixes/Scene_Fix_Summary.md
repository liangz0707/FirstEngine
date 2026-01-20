# Scene 找不到定义 - 修复总结

## 问题分析

程序报错 "Scene 找不到定义"，经过检查发现以下问题：

1. **FE_RESOURCES_API 宏问题**：
   - `FirstEngine_Resources` 是一个**静态库**（STATIC）
   - 但 `FE_RESOURCES_API` 宏在 Windows 上使用了 `__declspec(dllexport/dllimport)`
   - 静态库不需要导出/导入符号，应该定义为空

2. **SceneLoader.h 包含问题**：
   - `RenderContext.h` 包含了不存在的 `SceneLoader.h`
   - `SceneLoader` 类实际上定义在 `Scene.h` 中

## 修复内容

### 1. 修复 FE_RESOURCES_API 宏（`include/FirstEngine/Resources/Export.h`）

**修改前**：
```cpp
#ifdef _WIN32
    #ifdef FirstEngine_Resources_EXPORTS
        #define FE_RESOURCES_API __declspec(dllexport)
    #else
        #define FE_RESOURCES_API __declspec(dllimport)
    #endif
#else
    #define FE_RESOURCES_API __attribute__((visibility("default")))
#endif
```

**修改后**：
```cpp
#ifdef _WIN32
    // FirstEngine_Resources is a STATIC library, so we don't need __declspec(dllexport/dllimport)
    // Static libraries are linked directly into the executable/DLL, so symbols don't need to be exported
    #define FE_RESOURCES_API
#else
    #define FE_RESOURCES_API __attribute__((visibility("default")))
#endif
```

### 2. 修复 RenderContext.h 的包含（`include/FirstEngine/Renderer/RenderContext.h`）

**修改前**：
```cpp
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/SceneLoader.h"  // 这个文件不存在
#include "FirstEngine/Resources/ResourceManager.h"
```

**修改后**：
```cpp
#include "FirstEngine/Resources/Scene.h"  // SceneLoader is defined in Scene.h
#include "FirstEngine/Resources/ResourceManager.h"
```

## Scene 类的定义位置

- **头文件**：`include/FirstEngine/Resources/Scene.h`
- **实现文件**：`src/Resources/Scene.cpp`
- **命名空间**：`FirstEngine::Resources`
- **导出宏**：`FE_RESOURCES_API`（现在定义为空，因为静态库）

## 场景加载流程

### RenderContext 中的场景管理

1. **初始化场景**：
   - `InitializeEngine()` 或 `InitializeForWindow()` 中创建默认场景
   - `m_Scene = new FirstEngine::Resources::Scene("Editor Scene")` 或 `"Example Scene"`

2. **加载场景**：
   ```cpp
   bool RenderContext::LoadScene(const std::string& scenePath) {
       if (!m_Scene) {
           m_Scene = new FirstEngine::Resources::Scene("Editor Scene");
       }
       if (fs::exists(scenePath)) {
           if (FirstEngine::Resources::SceneLoader::LoadFromFile(scenePath, *m_Scene)) {
               return true;
           }
       }
       return false;
   }
   ```

3. **卸载场景**：
   ```cpp
   void RenderContext::UnloadScene() {
       if (m_Scene) {
           delete m_Scene;
           m_Scene = new FirstEngine::Resources::Scene("Editor Scene");
       }
   }
   ```

### SceneLoader 的使用

- **定义位置**：`include/FirstEngine/Resources/Scene.h`（第 281 行）
- **实现位置**：`src/Resources/Scene.cpp`
- **方法**：
  - `SceneLoader::LoadFromFile()` - 从文件加载场景
  - `SceneLoader::SaveToFile()` - 保存场景到文件
  - `SceneLoader::LoadFromJSON()` - 从 JSON 加载场景
  - `SceneLoader::SaveToJSON()` - 保存场景为 JSON

## 链接关系

- `FirstEngine_Renderer`（静态库）链接到 `FirstEngine_Resources`（静态库）
- `FirstEngine_Resources` 链接到 `FirstEngine_Renderer`（循环依赖，通过静态库解决）
- `FirstEngine_Core`（共享库）链接到 `FirstEngine_Editor`（静态库），后者链接到 `FirstEngine_Resources`

## 验证

修复后，Scene 类应该能够正常使用：
- ✅ Scene 类定义可见
- ✅ SceneLoader 方法可调用
- ✅ 场景加载流程正常
- ✅ 链接关系正确

## 注意事项

1. **静态库 vs 共享库**：
   - 静态库不需要 `__declspec(dllexport/dllimport)`
   - 共享库（DLL）才需要导出/导入符号

2. **循环依赖**：
   - `FirstEngine_Resources` 和 `FirstEngine_Renderer` 相互依赖
   - 通过将两者都设为静态库来解决循环依赖问题

3. **包含文件**：
   - `SceneLoader` 在 `Scene.h` 中定义，不需要单独的 `SceneLoader.h`
