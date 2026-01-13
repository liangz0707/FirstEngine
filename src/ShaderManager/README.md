# ShaderManager - Shader编译和转换命令行工具

## 功能

ShaderManager是一个命令行工具，提供以下功能：

1. **编译Shader**: 将GLSL/HLSL源码编译为SPIR-V
2. **转换Shader**: 将SPIR-V转换为GLSL/HLSL/MSL
3. **反射信息**: 显示shader的资源信息（Uniform Buffers、Samplers等）

## 使用方法

### 编译Shader

```bash
ShaderManager compile -i vertex.vert -o vertex.spv
ShaderManager compile -i fragment.frag -s fragment -o fragment.spv
ShaderManager compile -i shader.hlsl -l hlsl -e PSMain -o shader.spv
```

### 转换Shader

```bash
ShaderManager convert -i shader.spv -f glsl -o shader.glsl
ShaderManager convert -i shader.spv -f hlsl -o shader.hlsl
ShaderManager convert -i shader.spv -f msl -o shader.metal
```

### 反射信息

```bash
ShaderManager reflect -i shader.spv
```

## 选项说明

### Compile选项
- `-i, --input <file>`: 输入shader源码文件
- `-o, --output <file>`: 输出SPIR-V文件（默认：<input>.spv）
- `-s, --stage <stage>`: Shader阶段（vertex, fragment, geometry, compute等）
- `-l, --language <lang>`: 源码语言（glsl, hlsl）
- `-e, --entry <name>`: Entry point名称（默认：main）
- `-O, --optimize <level>`: 优化级别0-2（默认：0）
- `-g, --debug`: 生成调试信息
- `-D, --define <name[=val]>`: 定义宏

### Convert选项
- `-i, --input <file>`: 输入SPIR-V文件
- `-o, --output <file>`: 输出文件（默认：<input>.<ext>）
- `-f, --format <format>`: 目标格式（glsl, hlsl, msl）
- `-e, --entry <name>`: Entry point名称（默认：main）

### Reflect选项
- `-i, --input <file>`: 输入SPIR-V文件
- `--no-resources`: 隐藏所有资源
- `--no-ub`: 隐藏Uniform Buffers
- `--no-samplers`: 隐藏Samplers
- `--no-images`: 隐藏Images
- `--no-storage`: 隐藏Storage Buffers

## 示例

```bash
# 编译顶点着色器
ShaderManager compile -i shaders/vertex.vert -o shaders/vertex.spv

# 编译片段着色器（自动检测stage）
ShaderManager compile -i shaders/fragment.frag

# 编译HLSL着色器
ShaderManager compile -i shaders/pixel.hlsl -l hlsl -e PSMain

# 转换为GLSL
ShaderManager convert -i shader.spv -f glsl

# 转换为MSL（Metal）
ShaderManager convert -i shader.spv -f msl -o shader.metal

# 查看shader反射信息
ShaderManager reflect -i shader.spv
```
