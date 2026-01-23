# Vertex 结构体重构

## 概述

将 `Vertex` 结构体的定义从 `ModelLoader.h` 移动到 `MeshLoader.h`，因为 `Vertex` 是网格几何数据的核心结构，应该属于 `MeshLoader` 而不是 `ModelLoader`。

## 重构内容

### 1. 定义位置变更

**之前：**
- `Vertex` 定义在 `include/FirstEngine/Resources/ModelLoader.h`

**之后：**
- `Vertex` 定义在 `include/FirstEngine/Resources/MeshLoader.h`

### 2. 文件修改

#### `include/FirstEngine/Resources/MeshLoader.h`
- ✅ 添加了 `#include <glm/glm.hpp>` 用于 `glm::vec3`, `glm::vec2`, `glm::vec4`
- ✅ 将 `struct Vertex;` 前向声明改为完整定义
- ✅ 添加了 `Vertex` 结构体的完整定义：
  ```cpp
  FE_RESOURCES_API struct Vertex {
      glm::vec3 position;
      glm::vec3 normal;
      glm::vec2 texCoord;
      glm::vec4 tangent;  // xyz = tangent, w = handedness (1.0 or -1.0)
  };
  ```

#### `include/FirstEngine/Resources/ModelLoader.h`
- ✅ 移除了 `Vertex` 结构体的定义
- ✅ 添加了 `#include "FirstEngine/Resources/MeshLoader.h"` 来获取 `Vertex` 定义
- ✅ 添加了注释说明 `Vertex` 现在定义在 `MeshLoader.h` 中

#### `include/FirstEngine/Resources/MeshResource.h`
- ✅ 移除了 `struct Vertex;` 前向声明
- ✅ 添加了 `#include "FirstEngine/Resources/MeshLoader.h"` 来获取 `Vertex` 定义

#### `src/Resources/MeshLoader.cpp`
- ✅ 更新了注释，说明 `ModelLoader.h` 的包含仅用于 `Bone` 结构体

#### `src/Resources/ModelResource.cpp`
- ✅ 添加了 `#include "FirstEngine/Resources/MeshLoader.h"` 来获取 `Vertex` 定义（用于 `sizeof(Vertex)`）

## 依赖关系

### 新的包含关系

```
MeshLoader.h
  ├─ 定义 Vertex 结构体
  └─ 包含 glm/glm.hpp

ModelLoader.h
  ├─ 包含 MeshLoader.h (获取 Vertex 定义)
  └─ 定义 Mesh 结构体 (使用 Vertex)

MeshResource.h
  ├─ 包含 MeshLoader.h (获取 Vertex 定义)
  └─ 使用 Vertex 作为参数类型

ModelResource.cpp
  ├─ 包含 ModelLoader.h (获取 Mesh 定义)
  └─ 包含 MeshLoader.h (获取 Vertex 定义，用于 sizeof)
```

## 优势

1. **逻辑清晰**：`Vertex` 是网格几何数据的核心结构，放在 `MeshLoader.h` 中更符合职责划分
2. **减少耦合**：`ModelLoader` 不再需要直接定义 `Vertex`，只需包含 `MeshLoader.h`
3. **易于维护**：所有与网格几何相关的结构（`Vertex`, `Mesh`）现在有更清晰的归属

## 验证

所有使用 `Vertex` 的文件现在都正确包含了 `MeshLoader.h`：
- ✅ `MeshLoader.cpp` - 直接使用 `Vertex`
- ✅ `ModelLoader.cpp` - 通过 `ModelLoader.h` 间接获取 `Vertex`
- ✅ `ModelResource.cpp` - 直接包含 `MeshLoader.h` 用于 `sizeof(Vertex)`
- ✅ `MeshResource.cpp` - 通过 `MeshResource.h` 间接获取 `Vertex`

## 注意事项

- `ModelLoader.h` 中的 `Mesh` 结构体仍然使用 `Vertex`，因此需要包含 `MeshLoader.h`
- `ModelLoader.cpp` 中的 `LoadFromFile` 方法仍然使用 `Vertex`，通过 `ModelLoader.h` 的包含链获取
- 所有包含 `ModelLoader.h` 的文件现在也会间接包含 `MeshLoader.h`，这是可以接受的依赖关系
