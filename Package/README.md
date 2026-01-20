# FirstEngine 资源格式说明

本目录包含 FirstEngine 资源系统的示例文件和格式说明。

## 目录结构

```
Package/
├── Textures/          # 纹理资源示例
├── Materials/         # 材质资源示例
├── Meshes/           # 网格资源示例
├── Models/           # 模型资源示例
└── README.md         # 本文件
```

## 资源类型说明

### 1. Texture（纹理资源）

**文件格式**: XML + 图片文件

**XML 结构**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Texture>
    <Name>资源名称</Name>
    <ResourceID>资源ID（uint64）</ResourceID>
    <ImageFile>图片文件路径（相对于XML文件）</ImageFile>
    <Width>宽度（像素）</Width>
    <Height>高度（像素）</Height>
    <Channels>通道数（1=灰度, 3=RGB, 4=RGBA）</Channels>
    <HasAlpha>是否有Alpha通道（true/false）</HasAlpha>
</Texture>
```

**支持的图片格式**:
- JPEG (.jpg, .jpeg)
- PNG (.png)
- BMP (.bmp)
- TGA (.tga)
- DDS (.dds)
- TIFF (.tiff)
- HDR (.hdr)

**示例文件**: `Textures/example_texture.xml`

**说明**:
- `ImageFile` 可以是相对路径（相对于 XML 文件）或绝对路径
- 图片文件必须与 XML 文件在同一目录或子目录中
- `ResourceID` 必须是唯一的 64 位无符号整数

---

### 2. Material（材质资源）

**文件格式**: XML

**XML 结构**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Material>
    <Name>资源名称</Name>
    <ResourceID>资源ID（uint64）</ResourceID>
    <Shader>着色器名称</Shader>
    <Parameters>
        <Parameter name="参数名" type="类型">值</Parameter>
        <!-- 更多参数... -->
    </Parameters>
    <Textures>
        <Texture slot="纹理槽名称" resourceID="纹理资源ID"/>
        <!-- 更多纹理... -->
    </Textures>
    <Dependencies>
        <Dependency type="依赖类型" resourceID="依赖资源ID" slot="槽位" required="是否必需"/>
        <!-- 更多依赖... -->
    </Dependencies>
</Material>
```

**参数类型**:
- `float`: 浮点数，例如: `0.5`
- `vec2`: 2D向量，例如: `1.0 0.0`
- `vec3`: 3D向量，例如: `1.0 0.0 0.0`
- `vec4`: 4D向量，例如: `1.0 1.0 1.0 1.0`
- `int`: 整数，例如: `42`
- `bool`: 布尔值，例如: `true` 或 `false`
- `mat3`: 3x3矩阵，9个浮点数，例如: `1 0 0 0 1 0 0 0 1`
- `mat4`: 4x4矩阵，16个浮点数，例如: `1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1`

**依赖类型**:
- `Texture`: 纹理资源
- `Material`: 材质资源
- `Mesh`: 网格资源
- `Model`: 模型资源

**示例文件**: `Materials/example_material.xml`

**说明**:
- `Textures` 节点中的纹理引用会被自动转换为 `Dependencies`
- `slot` 用于标识纹理在材质中的用途（如 "Albedo", "Normal", "Metallic" 等）
- `required` 表示该依赖是否必需（如果为 false，加载失败不会导致整个材质加载失败）

---

### 3. Mesh（网格资源）

**文件格式**: XML + 几何文件（FBX/OBJ等）

**XML 结构**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Mesh>
    <Name>资源名称</Name>
    <ResourceID>资源ID（uint64）</ResourceID>
    <MeshFile>几何文件路径（相对于XML文件）</MeshFile>
    <VertexStride>顶点步长（字节数，可选）</VertexStride>
</Mesh>
```

**支持的几何格式**:
- FBX (.fbx)
- OBJ (.obj)
- DAE (.dae)
- 3DS (.3ds)
- BLEND (.blend)
- 以及 Assimp 支持的其他 40+ 种格式

**示例文件**: `Meshes/example_mesh.xml`

**说明**:
- `MeshFile` 指向包含实际几何数据的文件（FBX、OBJ 等）
- 几何文件必须与 XML 文件在同一目录或子目录中
- `VertexStride` 是可选的，如果不指定，将从几何文件推断
- Mesh 资源只包含单个网格的几何数据（顶点、索引、骨骼等）

---

### 4. Model（模型资源）

**文件格式**: XML（逻辑资源，不包含实际几何数据）

**XML 结构**:
```xml
<?xml version="1.0" encoding="UTF-8"?>
<Model>
    <Name>资源名称</Name>
    <ResourceID>资源ID（uint64）</ResourceID>
    <ModelFile></ModelFile>
    <Meshes>
        <Mesh index="网格索引" resourceID="网格资源ID"/>
        <!-- 更多网格... -->
    </Meshes>
    <Materials>
        <Material index="材质索引" resourceID="材质资源ID"/>
        <!-- 更多材质... -->
    </Materials>
    <Textures>
        <Texture slot="纹理槽名称" resourceID="纹理资源ID"/>
        <!-- 更多纹理... -->
    </Textures>
    <Dependencies>
        <Dependency type="依赖类型" resourceID="依赖资源ID" slot="槽位/索引" required="是否必需"/>
        <!-- 更多依赖... -->
    </Dependencies>
</Model>
```

**示例文件**: `Models/example_model.xml`

**说明**:
- Model 是**逻辑资源**，不包含实际的几何数据
- Model 是场景渲染单位，用于组合多个 Mesh 和 Material
- `ModelFile` 通常为空（Model 是逻辑资源，不需要源文件）
- `Meshes` 和 `Materials` 中的 `index` 用于标识网格/材质在模型中的位置
- `Textures` 用于模型级别的纹理（如 Lightmap）
- 所有引用都会被转换为 `Dependencies` 节点

---

## 资源加载流程

1. **ResourceManager** 通过 `ResourceID` 管理所有资源
2. **Loader** 负责从 XML 文件加载资源元数据和依赖关系
3. **Resource** 类负责加载实际数据和依赖资源
4. 依赖资源通过 `ResourceID` 引用，路径解析由 ResourceManager 内部处理

## 资源依赖关系

资源之间通过 `ResourceID` 建立依赖关系：

```
Model (4001)
├── Mesh (3001) ──→ MeshFile (example_mesh.fbx)
├── Mesh (3002)
├── Material (2001)
│   ├── Texture (1001) ──→ ImageFile (example_texture.png)
│   └── Texture (1002)
└── Texture (1003) ──→ Lightmap
```

## 注意事项

1. **ResourceID 唯一性**: 所有资源的 `ResourceID` 必须在整个系统中唯一
2. **路径解析**: 相对路径相对于 XML 文件所在目录解析
3. **依赖加载**: 依赖资源由 `Resource::Load` 方法自动加载，Loader 只负责记录依赖关系
4. **占位符文件**: 示例中的 `.fbx`、`.png` 等二进制文件是占位符，实际使用时需要替换为真实文件
5. **资源缓存**: ResourceManager 会缓存已加载的资源，避免重复加载

## 示例使用

```cpp
#include "FirstEngine/Resources/ResourceProvider.h"

using namespace FirstEngine::Resources;

// 获取 ResourceManager 单例
ResourceManager& rm = ResourceManager::GetInstance();

// 注册资源路径
ResourceID textureID = rm.GetIDManager().RegisterResource(
    "Textures/example_texture.xml", 
    ResourceType::Texture
);

// 加载资源
ResourceHandle handle = rm.Load(textureID);
if (handle.texture) {
    // 使用纹理资源
    uint32_t width = handle.texture->GetWidth();
    uint32_t height = handle.texture->GetHeight();
    const void* data = handle.texture->GetData();
}
```

## 更多信息

详细 API 文档请参考：
- `include/FirstEngine/Resources/ResourceProvider.h`
- `include/FirstEngine/Resources/TextureLoader.h`
- `include/FirstEngine/Resources/MaterialLoader.h`
- `include/FirstEngine/Resources/MeshLoader.h`
- `include/FirstEngine/Resources/ModelLoader.h`
