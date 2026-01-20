# Model XML 文件格式说明

## 正确的 Model XML 结构

Model XML 文件必须遵循以下结构，以便 `ResourceXMLParser::GetModelData()` 能够正确解析：

### 基本结构

```xml
<?xml version="1.0" encoding="UTF-8"?>
<Model>
    <Name>模型名称</Name>
    <ResourceID>资源ID（全局唯一）</ResourceID>
    <Meshes>
        <Mesh index="网格索引" resourceID="网格资源ID" />
        <!-- 可以有多个 Mesh -->
    </Meshes>
    <Materials>
        <Material index="材质索引" resourceID="材质资源ID" />
        <!-- 可以有多个 Material -->
    </Materials>
    <Textures>
        <!-- 可选：直接引用的纹理 -->
        <Texture slot="纹理槽名称" resourceID="纹理资源ID" />
    </Textures>
    <Dependencies>
        <!-- 依赖关系（可选，但建议包含） -->
        <Dependency type="Material" resourceID="材质ID" slot="索引" required="true" />
        <Dependency type="Mesh" resourceID="网格ID" slot="索引" required="true" />
        <Dependency type="Texture" resourceID="纹理ID" slot="槽名称" required="true" />
    </Dependencies>
</Model>
```

### 重要说明

1. **根节点必须是 `<Model>`**，不能是 `<Resource>`
2. **Model 是逻辑资源**，不包含实际的几何数据文件（如 .obj, .fbx）
   - Model 只包含 Mesh 和 Material 的引用
   - 实际的几何文件（.obj, .fbx）应该在 **Mesh XML 文件**中指定（`<MeshFile>`）
3. **`<Meshes>` 节点**：
   - 每个 `<Mesh>` 子节点必须有 `index` 和 `resourceID` 属性
   - `index` 是网格在模型中的索引位置
   - `resourceID` 是网格资源的全局唯一 ID
   - **不要使用** `name` 或 `materialID` 属性
4. **`<Materials>` 节点**：
   - 每个 `<Material>` 子节点必须有 `index` 和 `resourceID` 属性
   - `index` 是材质在模型中的索引位置（对应 Mesh 的 materialID）
   - `resourceID` 是材质资源的全局唯一 ID
5. **`<Textures>` 节点**（可选）：
   - 用于模型直接引用的纹理（如 lightmap）
   - 每个 `<Texture>` 有 `slot` 和 `resourceID` 属性
6. **`<Dependencies>` 节点**（可选但建议）：
   - 列出所有依赖的资源
   - 每个 `<Dependency>` 有 `type`、`resourceID`、`slot` 和 `required` 属性

### 资源层次结构

```
Model (逻辑资源)
  ├── Mesh (几何数据，从 .obj/.fbx 文件加载)
  │   └── MeshFile: cube.obj
  └── Material (材质数据)
      └── Texture (纹理数据)
```

- **Model**: 逻辑组合，只包含资源引用
- **Mesh**: 实际几何数据，从文件加载（在 Mesh XML 中指定 `<MeshFile>`）
- **Material**: 材质属性
- **Texture**: 纹理图像

### 示例文件

参考以下文件作为示例：
- `cube_model.xml` - Model 文件（只包含引用）
- `Meshes/cube_mesh.xml` - Mesh 文件（包含实际的 .obj 文件路径）

### 解析流程

1. `ResourceXMLParser::GetModelData()` 读取 Model XML 文件
2. 提取 `Meshes`、`Materials`、`Textures` 信息（只包含 ResourceID 引用）
3. `ModelLoader::Load()` 将这些信息转换为 `ResourceDependency` 并存储在 `metadata.dependencies` 中
4. `ModelResource::Load()` 根据依赖关系加载实际的 Mesh 和 Material 资源
5. Mesh 资源从自己的 XML 文件中读取 `<MeshFile>` 路径，然后加载实际的几何数据

### 常见错误

❌ **错误的结构**（包含 ModelFile）：
```xml
<Model>
    <ModelFile>cube.obj</ModelFile>  <!-- 错误：Model 不应该有文件路径 -->
    <Meshes>
        <Mesh index="0" resourceID="8" />
    </Meshes>
</Model>
```

✅ **正确的结构**：
```xml
<Model>
    <Meshes>
        <Mesh index="0" resourceID="8" />
    </Meshes>
    <Materials>
        <Material index="0" resourceID="4" />
    </Materials>
</Model>
```

✅ **Mesh XML 文件**（包含实际文件路径）：
```xml
<Mesh>
    <Name>CubeMesh</Name>
    <ResourceID>8</ResourceID>
    <MeshFile>cube.obj</MeshFile>  <!-- 正确：Mesh 包含文件路径 -->
    <VertexStride>32</VertexStride>
</Mesh>
```
