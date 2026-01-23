# 资源加载流程分析

## 整体架构

资源加载采用**统一接口 + 缓存机制**的设计：
- `ResourceManager`：单例，管理所有资源的加载、缓存和生命周期
- `Resource`类（ModelResource, MeshResource, MaterialResource, TextureResource）：实现具体的加载逻辑
- `Loader`类（ModelLoader, MeshLoader, MaterialLoader, TextureLoader）：负责从文件读取数据

## 加载流程

### 1. ResourceManager::Load(ResourceID id)

```
ResourceManager::Load(id)
├─ 检查缓存 (Get(id))
│  └─ 如果已缓存 → 增加引用计数 → 返回缓存的资源
└─ 未缓存 → 调用 LoadInternal(id)
```

**关键点：**
- 缓存检查在加载之前，避免重复加载
- 缓存命中时只增加引用计数，不重新加载

### 2. ResourceManager::LoadInternal(ResourceID id)

```
LoadInternal(id)
├─ 获取资源路径 (GetPathFromID)
├─ 确定资源类型 (GetTypeFromID / DetectResourceType)
├─ 创建Resource实例 (new ModelResource/MeshResource/etc.)
├─ 设置ResourceID到metadata
├─ 调用 Resource::Load(id) ← 关键步骤
│  └─ 这里会触发依赖加载
└─ 加载成功后存入缓存 (m_LoadedModels/Meshes/Materials/Textures)
```

**关键点：**
- 在调用 `Resource::Load` 之前，资源**尚未**存入缓存
- 这意味着在 `Resource::Load` 内部加载依赖时，如果依赖也依赖当前资源，可能导致问题

### 3. ModelResource::Load(ResourceID id)

```
ModelResource::Load(id)
├─ 调用 ModelLoader::Load(id)
│  └─ 返回 metadata（包含dependencies列表）
├─ 遍历 dependencies
│  └─ 对每个依赖调用 resourceManager.Load(dep.resourceID)
│     ├─ 如果依赖已缓存 → 直接返回（增加引用计数）
│     └─ 如果依赖未缓存 → 递归加载依赖
└─ 将加载的依赖存储到ModelResource中
```

**潜在问题：**
- 在 `LoadInternal` 中，资源在调用 `Resource::Load` 之前**尚未**存入缓存
- 如果依赖链中存在循环依赖，可能导致无限递归
- 但实际中，Model → Mesh/Material → Texture 的依赖链通常是单向的

### 4. MaterialResource::Load(ResourceID id)

```
MaterialResource::Load(id)
├─ 调用 MaterialLoader::Load(id)
│  └─ 返回 metadata（包含texture dependencies）
├─ 存储texture IDs到 m_TextureIDs
├─ 调用 LoadDependencies() ← 显式调用
│  └─ 遍历dependencies，加载texture资源
└─ 加载ShaderCollection
```

**问题：**
- `MaterialResource::Load` 中已经调用了 `LoadDependencies()`
- 但在 `ModelResource::Load` 中加载Material依赖时，也会触发 `MaterialResource::Load`
- 这可能导致 `LoadDependencies()` 被调用两次，但由于缓存机制，实际不会重复加载

### 5. LoadDependencies() 方法

**ModelResource::LoadDependencies()**
- 在 `ModelResource::Load` 中**没有**被显式调用
- 依赖加载直接在 `Load` 方法中完成（第84-143行）
- `LoadDependencies()` 方法存在但似乎未被使用

**MaterialResource::LoadDependencies()**
- 在 `MaterialResource::Load` 中被显式调用（第87行）
- 加载texture依赖

**MeshResource::LoadDependencies()**
- 空实现（Mesh通常没有依赖）

**TextureResource::LoadDependencies()**
- 需要查看实现

## 发现的问题

### 问题1：重复的依赖加载逻辑

**ModelResource::Load** 中：
- 第84-143行：直接在 `Load` 方法中加载依赖
- 第158-203行：`LoadDependencies()` 方法也实现了相同的逻辑

**分析：**
- `LoadDependencies()` 方法似乎未被使用
- 依赖加载逻辑重复，可能导致维护困难

### 问题2：缓存时机

**当前流程：**
```
LoadInternal(id)
├─ 创建Resource实例
├─ 调用 Resource::Load(id)  ← 此时资源未在缓存中
│  └─ Resource::Load 内部加载依赖
│     └─ 依赖调用 resourceManager.Load()
│        └─ 如果依赖也依赖当前资源，可能有问题
└─ 加载成功后存入缓存
```

**潜在问题：**
- 如果存在循环依赖，可能导致无限递归
- 但实际依赖链通常是：Model → Mesh/Material → Texture（单向）

### 问题3：LoadDependencies() 的调用时机不一致

- `MaterialResource::Load` 中显式调用了 `LoadDependencies()`
- `ModelResource::Load` 中没有调用 `LoadDependencies()`，而是直接在 `Load` 中处理
- 这种不一致可能导致混淆

## 建议的改进

### 1. 统一依赖加载逻辑

**方案A：所有Resource都在Load中直接加载依赖**
- 移除 `LoadDependencies()` 方法
- 在 `Load` 方法中统一处理依赖

**方案B：所有Resource都调用LoadDependencies()**
- 在 `Load` 方法中只收集依赖信息
- 统一在 `LoadDependencies()` 中加载依赖

### 2. 改进缓存机制

**方案：在创建Resource实例后立即存入缓存（标记为"加载中"）**
```cpp
ResourceHandle LoadInternal(ResourceID id) {
    // 创建Resource实例
    IResource* resource = new ModelResource();
    
    // 立即存入缓存（标记为加载中）
    m_LoadedModels[id] = static_cast<IModel*>(resource);
    
    // 调用Load（此时依赖加载时，如果依赖当前资源，可以从缓存获取）
    ResourceLoadResult result = resourceProvider->Load(id);
    
    // 如果加载失败，从缓存移除
    if (result != ResourceLoadResult::Success) {
        m_LoadedModels.erase(id);
        delete resource;
        return ResourceHandle();
    }
    
    return ResourceHandle(static_cast<ModelHandle>(resource));
}
```

这样可以：
- 防止循环依赖导致的无限递归
- 确保依赖加载时能正确获取当前资源

### 3. 添加加载状态跟踪

```cpp
enum class ResourceLoadState {
    NotLoaded,
    Loading,    // 正在加载中
    Loaded,
    Failed
};
```

在缓存中存储加载状态，防止重复加载和循环依赖。

## 当前流程的正确性

**优点：**
1. ✅ 缓存机制有效防止重复加载
2. ✅ 引用计数管理资源生命周期
3. ✅ 统一的加载接口

**潜在问题：**
1. ⚠️ 依赖加载逻辑重复（ModelResource）
2. ⚠️ 缓存时机可能导致循环依赖问题（虽然实际中不常见）
3. ⚠️ LoadDependencies() 调用时机不一致

## 已实施的改进

### 1. 统一依赖加载逻辑 ✅

**改进前：**
- `ModelResource::Load` 中直接在 `Load` 方法中加载依赖
- `LoadDependencies()` 方法存在但未被使用，逻辑不一致

**改进后：**
- `ModelResource::Load` 现在统一调用 `LoadDependencies()` 方法
- 所有Resource类型都使用相同的依赖加载模式
- `LoadDependencies()` 方法包含完整的依赖加载逻辑（Mesh, Material, Texture）

### 2. 改进缓存时机 ✅

**改进前：**
```
LoadInternal(id)
├─ 创建Resource实例
├─ 调用 Resource::Load(id)  ← 此时资源未在缓存中
│  └─ 如果依赖也依赖当前资源，可能有问题
└─ 加载成功后存入缓存
```

**改进后：**
```
LoadInternal(id)
├─ 创建Resource实例
├─ 立即存入缓存（引用计数=1） ← 关键改进
├─ 调用 Resource::Load(id)
│  └─ 如果依赖也依赖当前资源，可以从缓存获取
└─ 如果加载失败，从缓存移除并清理
```

**好处：**
- 防止循环依赖导致的无限递归
- 确保依赖加载时能正确获取当前资源（即使还在加载中）

### 3. 改进错误处理 ✅

- 添加了更详细的错误日志
- 改进了依赖加载失败时的处理
- 统一了slot名称处理逻辑

## 当前加载流程（改进后）

### ResourceManager::Load(ResourceID id)
```
1. 检查缓存 (Get(id))
   └─ 如果已缓存 → 增加引用计数 → 返回

2. 调用 LoadInternal(id)
   ├─ 创建Resource实例
   ├─ 立即存入缓存（防止循环依赖）
   ├─ 调用 Resource::Load(id)
   │  └─ Resource::Load 内部调用 LoadDependencies()
   │     └─ LoadDependencies() 调用 resourceManager.Load(dep.resourceID)
   │        └─ 如果依赖已缓存 → 直接返回
   │        └─ 如果依赖未缓存 → 递归加载
   └─ 如果加载失败 → 从缓存移除并清理
```

### ModelResource::Load(ResourceID id)
```
1. 调用 ModelLoader::Load(id) 获取metadata和依赖信息
2. 调用 LoadDependencies() 统一加载依赖
3. 标记为已加载
```

### MaterialResource::Load(ResourceID id)
```
1. 调用 MaterialLoader::Load(id) 获取metadata和依赖信息
2. 调用 LoadDependencies() 加载texture依赖
3. 加载ShaderCollection
4. 标记为已加载
```

## 总结

经过改进后，资源加载流程**更加健壮和一致**：

✅ **统一依赖加载方式**：所有Resource都使用 `LoadDependencies()` 方法
✅ **改进缓存时机**：在调用 `Load` 之前就将资源存入缓存，防止循环依赖
✅ **改进错误处理**：添加了详细的错误日志和失败处理

**关键改进点：**
1. 缓存机制现在在 `Load` 之前生效，防止循环依赖
2. 依赖加载逻辑统一，易于维护
3. 错误处理更加完善，便于调试
