# Vertex 结构与 Shader 输入匹配修复

## 问题描述

cube.obj 的几何结构和各种 vert shader 的 VertexInput 不匹配：
- C++ 端的 `Vertex` 结构只有：`position`, `normal`, `texCoord`
- 但 PBR.vert.hlsl 和 GeometryShader.vert.hlsl 需要：`position`, `normal`, `texCoord`, `tangent`

## 已实施的修复

### 1. 更新 Vertex 结构定义

**文件**: `include/FirstEngine/Resources/ModelLoader.h`

**修改前**:
```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};
```

**修改后**:
```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec4 tangent;  // xyz = tangent, w = handedness (1.0 or -1.0)
};
```

**新的 Vertex 结构大小**:
- position: `glm::vec3` = 12 bytes
- normal: `glm::vec3` = 12 bytes
- texCoord: `glm::vec2` = 8 bytes
- tangent: `glm::vec4` = 16 bytes
- **总计**: 48 bytes（之前是 32 bytes）

### 2. 更新 MeshLoader 以加载 Tangent

**文件**: `src/Resources/MeshLoader.cpp`

**修改内容**:
- 在加载顶点时，从 Assimp 的 `mTangents` 和 `mBitangents` 读取 tangent 数据
- 计算 handedness（w 分量）用于正确的法线贴图计算
- 如果模型没有 tangent，计算一个默认的 orthogonal tangent

**关键代码**:
```cpp
// Load tangent (required for PBR and Geometry shaders)
if (aiMesh->mTangents) {
    vertex.tangent = glm::vec4(
        aiMesh->mTangents[j].x,
        aiMesh->mTangents[j].y,
        aiMesh->mTangents[j].z,
        1.0f  // Default handedness
    );
    
    // Use bitangent to determine handedness if available
    if (aiMesh->mBitangents) {
        // Calculate handedness based on cross product
        glm::vec3 calculatedBitangent = glm::cross(normal, tangent);
        float handedness = (glm::dot(calculatedBitangent, bitangent) < 0.0f) ? -1.0f : 1.0f;
        vertex.tangent.w = handedness;
    }
} else {
    // Calculate fallback tangent if not present
    // ...
}
```

### 3. 更新 ModelLoader::LoadFromFile

**文件**: `src/Resources/ModelLoader.cpp`

**修改内容**:
- 同样添加了 tangent 加载逻辑
- 确保所有通过 ModelLoader 加载的模型都包含 tangent 数据

### 4. 更新 XML 文件中的 VertexStride

**修改的文件**:
- `build/Package/Meshes/cube_mesh.xml`: 32 → 48
- `build/Package/Meshes/plane_mesh.xml`: 32 → 48
- `build/Package/Meshes/sphere_mesh.xml`: 32 → 48

**注意**: 代码中已经使用 `sizeof(Vertex)` 作为默认值，所以即使 XML 中的值不正确，代码也会使用正确的值。但为了保持一致，已更新 XML 文件。

## Shader 输入要求总结

### 需要 Tangent 的 Shader

1. **PBR.vert.hlsl**
   ```hlsl
   struct VertexInput {
       float3 position : POSITION;
       float3 normal : NORMAL;
       float2 texCoord : TEXCOORD0;
       float4 tangent : TANGENT;  // ← 需要
   };
   ```

2. **GeometryShader.vert.hlsl**
   ```hlsl
   struct VertexInput {
       float3 position : POSITION;
       float3 normal : NORMAL;
       float2 texCoord : TEXCOORD0;
       float4 tangent : TANGENT;  // ← 需要
   };
   ```

### 不需要 Tangent 的 Shader

1. **Light.vert.hlsl** - 只需要 `position`
2. **IBL.vert.hlsl** - 需要 `position`, `normal`, `texCoord`
3. **SunLight.vert.hlsl** - 需要 `position`, `normal`, `texCoord`
4. **PostProcess.vert.hlsl** - 需要 `position`, `texCoord`
5. **Tonemapping.vert.hlsl** - 需要 `position`, `texCoord`

## Tangent 计算说明

### Handedness (W 分量)

Tangent 的 w 分量存储 handedness，用于正确的法线贴图计算：
- `1.0f`: 右手坐标系（normal × tangent = bitangent）
- `-1.0f`: 左手坐标系（normal × tangent = -bitangent）

### 计算方式

1. **如果模型有 tangent 和 bitangent**:
   - 使用 Assimp 计算的值
   - 通过比较 `cross(normal, tangent)` 和 `bitangent` 的方向来确定 handedness

2. **如果模型没有 tangent**:
   - 计算一个与 normal 正交的默认 tangent
   - 使用默认 handedness = 1.0f

## 验证

修复后，所有 mesh 资源（包括 cube.obj）现在都包含完整的顶点属性：
- ✅ position (vec3)
- ✅ normal (vec3)
- ✅ texCoord (vec2)
- ✅ tangent (vec4)

这确保了所有 vert shader 都能正确接收所需的顶点输入。

## 注意事项

1. **向后兼容性**: 如果旧的模型文件没有 tangent，代码会计算一个默认值
2. **性能**: 使用 Assimp 的 `aiProcess_CalcTangentSpace` 标志自动计算 tangent，性能良好
3. **正确性**: Handedness 的计算确保了法线贴图的正确性
