# FirstEngine 场景管理系统文档

## 概述

FirstEngine 场景管理系统提供了完整的场景数据组织、空间索引、视锥剔除和渲染批处理功能。系统采用现代游戏引擎的设计模式，包括：

- **实体-组件系统（ECS）**：灵活的实体和组件架构
- **八叉树空间索引**：高效的场景查询和剔除
- **视锥剔除**：自动剔除视锥外的对象
- **渲染批处理**：按材质和管线状态自动批处理 DrawCall

## 核心组件

### 1. 场景数据结构

#### Scene（场景）
场景是场景图的根节点，管理所有实体和空间索引。

```cpp
#include "FirstEngine/Resources/Scene.h"

// 创建场景
Resources::Scene scene("MyScene");

// 创建实体
auto* entity = scene.CreateEntity("Player");
```

#### Entity（实体）
实体是场景中的对象，包含变换信息和组件。

```cpp
// 获取变换
auto& transform = entity->GetTransform();
transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
transform.scale = glm::vec3(1.0f);

// 实体层次结构
entity->SetParent(parentEntity);
```

#### Component（组件）
组件是实体的功能模块：

- **MeshComponent**：网格渲染组件
- **LightComponent**：光源组件
- **EffectComponent**：特效/粒子系统组件

```cpp
// 添加网格组件
auto* mesh = entity->AddComponent<Resources::MeshComponent>();
mesh->SetModel("models/player.fbx");
mesh->SetMaterial("PlayerMaterial");

// 添加光源组件
auto* light = entity->AddComponent<Resources::LightComponent>();
light->SetType(Resources::LightType::Point);
light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
light->SetIntensity(1.0f);
light->SetRange(10.0f);
```

### 2. 空间索引（八叉树）

场景使用八叉树进行空间索引，支持高效的空间查询：

```cpp
// 查询指定区域内的实体
AABB queryBounds(glm::vec3(-10.0f), glm::vec3(10.0f));
auto entities = scene.QueryBounds(queryBounds);

// 视锥查询（用于剔除）
glm::mat4 viewProj = projMatrix * viewMatrix;
auto visibleEntities = scene.QueryFrustum(viewProj);

// 射线查询
auto hitEntities = scene.QueryRay(origin, direction, maxDistance);
```

### 3. 渲染系统

#### SceneRenderer（场景渲染器）
场景渲染器负责将场景数据转换为渲染命令：

```cpp
#include "FirstEngine/Renderer/SceneRenderer.h"

// 创建场景渲染器
Renderer::SceneRenderer renderer(device);
renderer.SetScene(&scene);

// 渲染场景
glm::mat4 viewMatrix = camera.GetViewMatrix();
glm::mat4 projMatrix = camera.GetProjectionMatrix();
renderer.Render(commandBuffer, viewMatrix, projMatrix);
```

#### RenderQueue（渲染队列）
渲染队列自动管理 DrawCall 批处理：

```cpp
Renderer::RenderQueue renderQueue;

// 构建渲染队列（自动进行视锥剔除）
renderer.BuildRenderQueue(viewMatrix, projMatrix, renderQueue);

// 获取批处理后的渲染项
const auto& batches = renderQueue.GetBatches();
for (const auto& batch : batches) {
    const auto& items = batch.GetItems();
    // 渲染批处理中的项
}
```

### 4. 视锥剔除

系统自动进行视锥剔除，可以手动控制：

```cpp
// 启用/禁用视锥剔除
renderer.SetFrustumCullingEnabled(true);

// 启用遮挡剔除（需要 GPU 查询支持）
renderer.SetOcclusionCullingEnabled(false);

// 获取统计信息
size_t visible = renderer.GetVisibleEntityCount();
size_t culled = renderer.GetCulledEntityCount();
```

### 5. DrawCall 组织

渲染系统自动按以下顺序组织 DrawCall：

1. **按管线状态分组**：相同管线的 DrawCall 分组
2. **按材质分组**：相同材质的 DrawCall 分组
3. **按深度排序**：优化渲染顺序（前到后用于不透明，后到前用于透明）

```cpp
// RenderItem 结构包含所有渲染所需信息
Renderer::RenderItem item;
item.pipeline = pipeline;
item.vertexBuffer = vertexBuffer;
item.indexBuffer = indexBuffer;
item.worldMatrix = worldMatrix;
item.materialName = "MaterialName";
```

## 使用示例

### 完整场景创建和渲染示例

```cpp
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Renderer/SceneRenderer.h"

// 1. 创建场景
Resources::Scene scene("MyGameScene");

// 2. 创建实体并添加组件
auto* player = scene.CreateEntity("Player");
player->GetTransform().position = glm::vec3(0.0f, 0.0f, 0.0f);

auto* playerMesh = player->AddComponent<Resources::MeshComponent>();
playerMesh->SetModel("models/player.fbx");
playerMesh->SetMaterial("PlayerMaterial");

// 3. 添加光源
auto* lightEntity = scene.CreateEntity("PointLight");
lightEntity->GetTransform().position = glm::vec3(5.0f, 5.0f, 5.0f);

auto* light = lightEntity->AddComponent<Resources::LightComponent>();
light->SetType(Resources::LightType::Point);
light->SetColor(glm::vec3(1.0f, 1.0f, 1.0f));
light->SetIntensity(2.0f);
light->SetRange(20.0f);

// 4. 创建场景渲染器
Renderer::SceneRenderer renderer(device);
renderer.SetScene(&scene);

// 5. 在渲染循环中渲染场景
void OnRender() {
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    
    commandBuffer->Begin();
    renderer.Render(commandBuffer, view, proj);
    commandBuffer->End();
}
```

## 场景保存和加载

场景可以保存为 JSON 格式（需要实现 JSON 序列化库，如 nlohmann/json）：

```cpp
// 保存场景
Resources::SceneLoader::SaveToFile("scenes/myscene.json", scene);

// 加载场景
Resources::Scene scene;
Resources::SceneLoader::LoadFromFile("scenes/myscene.json", scene);
```

## 性能优化建议

1. **八叉树重建**：场景修改后会自动标记需要重建，在适当时机调用 `RebuildOctree()`
2. **批处理**：系统自动批处理相同管线和材质的 DrawCall，减少状态切换
3. **视锥剔除**：始终启用视锥剔除以提升性能
4. **实体激活状态**：使用 `SetActive(false)` 禁用不需要的实体，它们不会被渲染

## 扩展性

系统设计支持扩展：

- **自定义组件**：继承 `Component` 类创建新组件类型
- **自定义剔除**：扩展 `CullingSystem` 实现自定义剔除逻辑
- **渲染通道**：可以创建多个 `SceneRenderer` 实例用于不同渲染通道（阴影、反射等）

## 架构说明

```
Scene (场景)
├── Entity (实体)
│   ├── Transform (变换)
│   ├── MeshComponent (网格)
│   ├── LightComponent (光源)
│   └── EffectComponent (特效)
│
Octree (八叉树空间索引)
├── 自动空间划分
├── 快速查询
└── 视锥剔除
│
SceneRenderer (场景渲染器)
├── 视锥剔除
├── 构建渲染队列
└── 执行渲染
│
RenderQueue (渲染队列)
├── 自动批处理
├── 排序优化
└── DrawCall 组织
```

## 注意事项

1. **资源管理**：模型和纹理资源需要预先加载，`MeshComponent` 会引用这些资源
2. **线程安全**：当前实现不是线程安全的，多线程访问需要加锁
3. **内存管理**：实体和组件使用智能指针管理，无需手动释放
4. **JSON 序列化**：当前 `SceneLoader` 的 JSON 实现是占位符，需要集成 JSON 库（如 nlohmann/json）

## 未来改进

- [ ] 实现完整的 JSON 序列化/反序列化
- [ ] 添加 GPU 遮挡剔除支持
- [ ] 实现 LOD（细节层次）系统
- [ ] 添加场景实例化支持
- [ ] 实现场景分层渲染（前景、中景、背景）
- [ ] 添加场景预计算光照支持
