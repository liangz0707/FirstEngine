# Shader 编译器模块说明

## 概述

Shader模块已集成 **SPIRV-Cross**，这是一个强大的shader交叉编译库，支持：
- SPIR-V 到 GLSL 转换
- SPIR-V 到 HLSL 转换
- SPIR-V 到 MSL (Metal Shading Language) 转换
- Shader反射（通过AST访问资源信息）

## 第三方库

### SPIRV-Cross
- **官方仓库**: https://github.com/KhronosGroup/SPIRV-Cross
- **版本**: sdk-1.3.261.1
- **功能**: 
  - 将SPIR-V字节码转换为多种shader语言
  - 提供AST（抽象语法树）访问接口
  - Shader资源反射和分析

## 使用方法

### 1. 创建Shader编译器

```cpp
#include "FirstEngine/Shader/ShaderCompiler.h"

// 从SPIR-V文件创建
FirstEngine::Shader::ShaderCompiler compiler("shaders/vert.spv");

// 或从SPIR-V二进制数据创建
std::vector<uint32_t> spirv_data = LoadSPIRVFile("shaders/vert.spv");
FirstEngine::Shader::ShaderCompiler compiler(spirv_data);
```

### 2. 转换到目标语言

```cpp
// 转换为GLSL
std::string glsl_code = compiler.CompileToGLSL("main");

// 转换为HLSL
std::string hlsl_code = compiler.CompileToHLSL("main");

// 转换为MSL (Metal)
std::string msl_code = compiler.CompileToMSL("main");
```

### 3. 获取Shader反射信息（AST访问）

```cpp
// 获取完整的反射数据
auto reflection = compiler.GetReflection();

// 访问Uniform Buffers
for (const auto& ub : reflection.uniform_buffers) {
    std::cout << "UB: " << ub.name 
              << " Set: " << ub.set 
              << " Binding: " << ub.binding 
              << " Size: " << ub.size << std::endl;
    
    // 访问成员（从AST获取）
    for (const auto& member : ub.members) {
        std::cout << "  Member: " << member.name 
                  << " Size: " << member.size << std::endl;
    }
}

// 获取Samplers
auto samplers = compiler.GetSamplers();

// 获取Images
auto images = compiler.GetImages();

// 获取Storage Buffers
auto storage_buffers = compiler.GetStorageBuffers();
```

### 4. 设置编译器选项

```cpp
// 设置GLSL版本（例如：330, 430, 450）
compiler.SetGLSLVersion(450);

// 设置HLSL Shader Model（例如：50 表示 Shader Model 5.0）
compiler.SetHLSLShaderModel(50);

// 设置MSL版本（例如：20000 表示 MSL 2.0）
compiler.SetMSLVersion(20000);
```

## AST（抽象语法树）访问

SPIRV-Cross内部使用AST来表示SPIR-V代码。通过反射API，你可以访问：

- **Uniform Buffers**: 通过`GetUniformBuffers()`或`GetReflection().uniform_buffers`
- **Samplers**: 通过`GetSamplers()`或`GetReflection().samplers`
- **Images**: 通过`GetImages()`或`GetReflection().images`
- **Storage Buffers**: 通过`GetStorageBuffers()`或`GetReflection().storage_buffers`
- **Stage Inputs/Outputs**: 通过`GetReflection().stage_inputs/outputs`

### 高级AST访问

如果需要更底层的AST访问，可以使用：

```cpp
void* internal_compiler = compiler.GetInternalCompiler();
// 可以将它转换为 spirv_cross::Compiler* 进行高级操作
```

## 完整示例

```cpp
#include "FirstEngine/Shader/ShaderCompiler.h"
#include <iostream>

int main() {
    try {
        // 加载并编译shader
        FirstEngine::Shader::ShaderCompiler compiler("shaders/vert.spv");
        
        // 设置选项
        compiler.SetGLSLVersion(450);
        compiler.SetHLSLShaderModel(50);
        
        // 转换为不同语言
        std::string glsl = compiler.CompileToGLSL("main");
        std::string hlsl = compiler.CompileToHLSL("main");
        std::string msl = compiler.CompileToMSL("main");
        
        // 获取反射信息
        auto reflection = compiler.GetReflection();
        
        std::cout << "Entry Point: " << reflection.entry_point << std::endl;
        std::cout << "Uniform Buffers: " << reflection.uniform_buffers.size() << std::endl;
        
        for (const auto& ub : reflection.uniform_buffers) {
            std::cout << "  " << ub.name 
                      << " (Set: " << ub.set 
                      << ", Binding: " << ub.binding << ")" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    
    return 0;
}
```

## 构建要求

- CMake 3.20+
- C++17
- SPIRV-Cross（通过FetchContent自动下载）

## 注意事项

1. **SPIR-V输入**: 编译器需要有效的SPIR-V字节码作为输入
2. **Entry Point**: 确保指定的entry point在SPIR-V中存在
3. **资源绑定**: 转换后的shader会保留原始的descriptor set和binding信息
4. **平台特定**: MSL和HLSL转换可能包含平台特定的优化
