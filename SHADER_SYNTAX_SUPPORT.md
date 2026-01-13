# Shader 语法支持完整说明

## ✅ 当前支持的Shader语法

### 1. **GLSL (OpenGL Shading Language)**
- ✅ **完整支持** - 通过 glslang 编译为 SPIR-V
- **支持版本**: GLSL 4.50+ (Vulkan兼容)
- **支持阶段**:
  - Vertex Shader (`.vert`, `.vertex`, `.vs`)
  - Fragment Shader (`.frag`, `.fragment`, `.fs`)
  - Geometry Shader (`.geom`, `.geometry`, `.gs`)
  - Compute Shader (`.comp`, `.compute`, `.cs`)
  - Tessellation Control (`.tesc`, `.tesscontrol`)
  - Tessellation Evaluation (`.tese`, `.tesseval`)

### 2. **HLSL (High-Level Shading Language)**
- ✅ **基本支持** - 通过 glslang 编译为 SPIR-V
- **注意**: glslang的HLSL支持可能有限制，完整HLSL支持需要DXC
- **支持阶段**: 同上GLSL阶段
- **文件扩展名**: `.hlsl`, `.fx`, `.fxh`

### 3. **SPIR-V (Standard Portable Intermediate Representation)**
- ✅ **完整支持** - 可以直接加载和使用SPIR-V字节码
- **文件扩展名**: `.spv`

## 🔄 转换方向支持

### ✅ GLSL/HLSL → SPIR-V
使用 `ShaderSourceCompiler` 类：
```cpp
FirstEngine::Shader::ShaderSourceCompiler compiler;
auto result = compiler.CompileGLSL(glsl_source, options);
// 或
auto result = compiler.CompileHLSL(hlsl_source, options);
```

### ✅ SPIR-V → GLSL/HLSL/MSL
使用 `ShaderCompiler` 类：
```cpp
FirstEngine::Shader::ShaderCompiler compiler(spirv_filepath);
std::string glsl = compiler.CompileToGLSL("main");
std::string hlsl = compiler.CompileToHLSL("main");
std::string msl = compiler.CompileToMSL("main");
```

## 📚 使用的第三方库

### 1. **glslang** (GLSL/HLSL → SPIR-V)
- **仓库**: https://github.com/KhronosGroup/glslang
- **版本**: 13.0.0 (通过FetchContent自动下载)
- **功能**: 
  - GLSL到SPIR-V编译（完整支持）
  - HLSL到SPIR-V编译（基本支持）
  - Vulkan语义支持

### 2. **SPIRV-Cross** (SPIR-V → GLSL/HLSL/MSL)
- **仓库**: https://github.com/KhronosGroup/SPIRV-Cross
- **版本**: sdk-1.3.261.1
- **功能**: 
  - SPIR-V到GLSL/HLSL/MSL转换
  - Shader反射和AST访问

## 💻 使用示例

### 示例1: 编译GLSL到SPIR-V

```cpp
#include "FirstEngine/Shader/ShaderSourceCompiler.h"

// GLSL源码
const char* vertex_shader = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = vec4(inPosition, 1.0);
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

FirstEngine::Shader::ShaderSourceCompiler compiler;
FirstEngine::Shader::CompileOptions options;
options.stage = FirstEngine::Shader::ShaderStage::Vertex;
options.language = FirstEngine::Shader::ShaderSourceLanguage::GLSL;
options.optimization_level = 1; // 性能优化

auto result = compiler.CompileGLSL(vertex_shader, options);

if (result.success) {
    // 保存SPIR-V
    FirstEngine::Shader::ShaderSourceCompiler::SaveSPIRV(
        result.spirv_code, "vertex.spv");
}
```

### 示例2: 从文件自动编译

```cpp
FirstEngine::Shader::ShaderSourceCompiler compiler;
auto result = compiler.CompileFromFileAuto("shaders/vertex.vert");
// 自动检测文件类型和shader stage
```

### 示例3: 使用宏定义

```cpp
FirstEngine::Shader::CompileOptions options;
options.stage = FirstEngine::Shader::ShaderStage::Vertex;
options.defines.push_back({"USE_TEXTURE", "1"});
options.defines.push_back({"MAX_LIGHTS", "4"});

auto result = compiler.CompileGLSL(shader_source, options);
```

### 示例4: 转换SPIR-V到其他语言

```cpp
// 从SPIR-V转换到GLSL
FirstEngine::Shader::ShaderCompiler compiler("vertex.spv");
std::string glsl_code = compiler.CompileToGLSL("main");

// 转换到HLSL
std::string hlsl_code = compiler.CompileToHLSL("main");

// 转换到MSL (Metal)
std::string msl_code = compiler.CompileToMSL("main");
```

## 🎯 工作流程建议

### 开发时工作流
1. **编写GLSL/HLSL源码** → 保存为`.glsl`或`.hlsl`文件
2. **运行时编译** → 使用`ShaderSourceCompiler`编译为SPIR-V
3. **加载SPIR-V** → 使用`ShaderLoader`加载SPIR-V到Vulkan

### 发布时工作流
1. **预编译shader** → 将GLSL/HLSL编译为`.spv`文件
2. **打包SPIR-V** → 将`.spv`文件打包到资源目录
3. **运行时加载** → 直接加载`.spv`文件（更快）

## ⚠️ 注意事项

1. **HLSL支持**: glslang的HLSL支持可能有限制，如需完整HLSL支持，考虑集成DXC（DirectX Shader Compiler）
2. **MSL转换**: MSL主要用于Metal，通常不需要转换为SPIR-V（Metal使用MSL直接）
3. **优化级别**: 
   - 0 = 无优化（开发时便于调试）
   - 1 = 性能优化（推荐用于发布）
   - 2 = 大小优化（移动平台推荐）
4. **调试信息**: 开启`generate_debug_info`会增加SPIR-V大小，但有助于调试

## 🔮 未来可能的扩展

- [ ] 集成DXC（DirectX Shader Compiler）获得完整HLSL支持
- [ ] 支持WGSL（WebGPU Shading Language）
- [ ] Shader hot-reload功能
- [ ] Shader预编译工具（命令行）
