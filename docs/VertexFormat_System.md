# 灵活的 Vertex 格式系统

## 概述

实现了灵活的 Vertex 格式系统，支持多种顶点属性组合，并能自动匹配 Shader 的 stage inputs。

## 主要改进

### 1. 移除 XML 中的 vertexStride 存储

**之前：**
- `vertexStride` 存储在 XML 文件中
- 从 XML 读取 `vertexStride`，如果不存在则使用 `sizeof(Vertex)`

**之后：**
- `vertexStride` 不再存储在 XML 中
- 从 mesh 文件的实际数据计算 `vertexStride`
- XML 只存储 mesh 文件路径

### 2. 灵活的 Vertex 格式系统

#### VertexFormat 类

定义了 `VertexFormat` 类来描述顶点的布局：

```cpp
class VertexFormat {
    // 添加属性
    void AddAttribute(VertexAttributeType type, uint32_t location);
    
    // 检查是否有特定属性
    bool HasAttribute(VertexAttributeType type) const;
    
    // 获取总 stride
    uint32_t GetStride() const;
    
    // 匹配 shader 需求
    bool MatchesShaderInputs(const std::vector<VertexAttributeType>& requiredAttributes) const;
};
```

#### 支持的顶点属性类型

- `Position` (必需) - float3
- `Normal` (可选) - float3
- `TexCoord0` (可选) - float2
- `TexCoord1` (可选) - float2
- `Tangent` (可选) - float4 (xyz=tangent, w=handedness)
- `Color0` (可选) - float4

#### 预定义的常见格式

- `CreatePositionOnly()` - 仅位置
- `CreatePositionNormal()` - 位置 + 法线
- `CreatePositionTexCoord()` - 位置 + 纹理坐标
- `CreatePositionNormalTexCoord()` - 位置 + 法线 + 纹理坐标
- `CreatePositionNormalTexCoordTangent()` - 位置 + 法线 + 纹理坐标 + 切线
- `CreatePositionNormalTexCoord0TexCoord1()` - 位置 + 法线 + 两个纹理坐标
- `CreatePositionNormalTexCoord0TexCoord1Tangent()` - 完整格式

#### 自动检测格式

`CreateFromMeshData()` 根据 mesh 文件的实际数据自动创建格式：

```cpp
VertexFormat format = VertexFormat::CreateFromMeshData(
    hasNormals,      // 是否有法线
    hasTexCoords0,   // 是否有第一个纹理坐标
    hasTexCoords1,   // 是否有第二个纹理坐标
    hasTangents,     // 是否有切线
    hasColors0       // 是否有颜色
);
```

### 3. MeshLoader 的改进

#### LoadResult 结构变更

**之前：**
```cpp
struct LoadResult {
    std::vector<Vertex> vertices;  // 固定格式
    uint32_t vertexStride = 0;
    // ...
};
```

**之后：**
```cpp
struct LoadResult {
    std::vector<uint8_t> vertexData;   // 灵活的原始数据
    VertexFormat vertexFormat;         // 格式描述符
    uint32_t vertexCount = 0;          // 顶点数量
    // ...
    
    // 向后兼容：转换为旧格式
    std::vector<Vertex> GetLegacyVertices() const;
};
```

#### 加载逻辑

1. 从 mesh 文件检测可用的顶点属性
2. 根据检测结果创建 `VertexFormat`
3. 将顶点数据存储为原始字节数组
4. `vertexStride` 从 `VertexFormat` 计算得出

### 4. Shader 匹配机制

#### VertexShaderMatcher 类

实现了 `VertexShaderMatcher` 类来匹配 Vertex 格式和 Shader 输入：

```cpp
struct MatchResult {
    bool isCompatible;                    // 是否兼容
    std::vector<uint32_t> locationMapping;  // 位置映射
    std::vector<std::string> missingAttributes;  // 缺失的属性
    std::vector<std::string> unusedAttributes;   // 未使用的属性
    std::string errorMessage;
};

MatchResult Match(const VertexFormat& vertexFormat, 
                 const Shader::ShaderReflection& shaderReflection);
```

#### 匹配策略

1. **提取 Shader 需求**：从 shader reflection 中提取所需的顶点属性
2. **名称匹配**：通过 shader input 名称推断属性类型
   - `position`, `pos` → Position
   - `normal`, `norm` → Normal
   - `texcoord0`, `uv0` → TexCoord0
   - `texcoord1`, `uv1` → TexCoord1
   - `tangent` → Tangent
   - `color0`, `color` → Color0
3. **位置匹配**：如果名称不明确，使用 location 作为提示
   - location 0 → Position
   - location 1 → Normal
   - location 2 → TexCoord0
   - 等等
4. **兼容性检查**：检查 Vertex 格式是否包含所有 Shader 需要的属性

## 使用示例

### 加载 Mesh

```cpp
MeshLoader::LoadResult result = MeshLoader::Load(meshID);

// 获取格式信息
const VertexFormat& format = result.vertexFormat;
uint32_t stride = format.GetStride();
uint32_t vertexCount = result.vertexCount;

// 访问原始数据
const uint8_t* vertexData = result.vertexData.data();
```

### 匹配 Shader

```cpp
Shader::ShaderCompiler compiler("shader.spv");
Shader::ShaderReflection reflection = compiler.GetReflection();

VertexShaderMatcher::MatchResult match = 
    VertexShaderMatcher::Match(result.vertexFormat, reflection);

if (match.isCompatible) {
    // 可以使用此 mesh 和 shader
    // match.locationMapping 提供了位置映射
} else {
    // 不兼容
    std::cerr << match.errorMessage << std::endl;
    // match.missingAttributes 列出了缺失的属性
}
```

### 向后兼容

```cpp
// 如果格式匹配旧 Vertex 结构，可以转换
std::vector<Vertex> legacyVertices = result.GetLegacyVertices();
if (!legacyVertices.empty()) {
    // 使用旧格式
}
```

## 文件变更

### 新增文件

1. `include/FirstEngine/Resources/VertexFormat.h` - Vertex 格式定义
2. `src/Resources/VertexFormat.cpp` - Vertex 格式实现
3. `include/FirstEngine/Resources/VertexShaderMatcher.h` - Shader 匹配器
4. `src/Resources/VertexShaderMatcher.cpp` - Shader 匹配器实现

### 修改文件

1. `include/FirstEngine/Resources/MeshLoader.h`
   - 更新 `LoadResult` 结构
   - 添加 `GetLegacyVertices()` 方法
   - 移除 `Save()` 中的 `vertexStride` 参数

2. `src/Resources/MeshLoader.cpp`
   - 重写加载逻辑以支持灵活格式
   - 实现 `GetLegacyVertices()` 方法

3. `include/FirstEngine/Resources/ResourceXMLParser.h`
   - 更新 `MeshData` 结构，移除 `vertexStride`

4. `src/Resources/ResourceXMLParser.cpp`
   - 移除 XML 中 `vertexStride` 的读取和保存

5. `src/Resources/CMakeLists.txt`
   - 添加新源文件

## 优势

1. **灵活性**：支持多种顶点属性组合，适应不同的 mesh 文件
2. **自动检测**：根据 mesh 文件的实际数据自动创建格式
3. **Shader 匹配**：自动检查 Vertex 格式是否与 Shader 兼容
4. **向后兼容**：保留旧 `Vertex` 结构，提供转换方法
5. **数据驱动**：不再需要在 XML 中硬编码 `vertexStride`

## 注意事项

1. **向后兼容性**：旧代码使用 `GetLegacyVertices()` 可以继续工作
2. **性能**：原始字节数据存储，避免不必要的转换
3. **扩展性**：可以轻松添加新的顶点属性类型
4. **Shader 匹配**：匹配基于名称和位置，可能需要根据实际 shader 调整

## 未来改进

1. 支持更多顶点属性类型（如骨骼权重、颜色通道等）
2. 改进 shader 匹配算法，支持更复杂的命名约定
3. 添加格式验证和错误报告
4. 支持顶点属性的重新排序和打包优化
