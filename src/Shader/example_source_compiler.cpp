// ShaderSourceCompiler 使用示例
// 展示如何将GLSL/HLSL源码编译为SPIR-V

#include "FirstEngine/Shader/ShaderSourceCompiler.h"
#include <iostream>
#include <fstream>

void ExampleCompileGLSL() {
    // GLSL顶点着色器源码示例
    const char* vertex_shader_src = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
)";

    FirstEngine::Shader::ShaderSourceCompiler compiler;
    FirstEngine::Shader::CompileOptions options;
    options.stage = FirstEngine::Shader::ShaderStage::Vertex;
    options.language = FirstEngine::Shader::ShaderSourceLanguage::GLSL;
    options.entry_point = "main";
    options.optimization_level = 1; // 性能优化
    options.generate_debug_info = false;
    
    // 编译GLSL到SPIR-V
    auto result = compiler.CompileGLSL(vertex_shader_src, options);
    
    if (result.success) {
        std::cout << "✅ GLSL编译成功！" << std::endl;
        std::cout << "生成的SPIR-V大小: " << result.spirv_code.size() << " words" << std::endl;
        
        // 保存SPIR-V到文件
        if (FirstEngine::Shader::ShaderSourceCompiler::SaveSPIRV(result.spirv_code, "vertex.spv")) {
            std::cout << "✅ SPIR-V已保存到 vertex.spv" << std::endl;
        }
    } else {
        std::cerr << "❌ GLSL编译失败:" << std::endl;
        std::cerr << result.error_message << std::endl;
    }
    
    // 显示警告
    if (!result.warnings.empty()) {
        std::cout << "⚠️  警告:" << std::endl;
        for (const auto& warning : result.warnings) {
            std::cout << "  " << warning << std::endl;
        }
    }
}

void ExampleCompileHLSL() {
    // HLSL像素着色器源码示例
    const char* pixel_shader_src = R"(
struct PSInput {
    float4 position : SV_POSITION;
    float3 color : COLOR;
    float2 texCoord : TEXCOORD;
};

Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

float4 PSMain(PSInput input) : SV_TARGET {
    float4 texColor = g_Texture.Sample(g_Sampler, input.texCoord);
    return float4(input.color * texColor.rgb, texColor.a);
}
)";

    FirstEngine::Shader::ShaderSourceCompiler compiler;
    FirstEngine::Shader::CompileOptions options;
    options.stage = FirstEngine::Shader::ShaderStage::Fragment;
    options.language = FirstEngine::Shader::ShaderSourceLanguage::HLSL;
    options.entry_point = "PSMain"; // HLSL使用自定义entry point
    options.optimization_level = 1;
    
    // 编译HLSL到SPIR-V
    auto result = compiler.CompileHLSL(pixel_shader_src, options);
    
    if (result.success) {
        std::cout << "✅ HLSL编译成功！" << std::endl;
        std::cout << "生成的SPIR-V大小: " << result.spirv_code.size() << " words" << std::endl;
        
        // 保存SPIR-V到文件
        FirstEngine::Shader::ShaderSourceCompiler::SaveSPIRV(result.spirv_code, "pixel.spv");
    } else {
        std::cerr << "❌ HLSL编译失败:" << std::endl;
        std::cerr << result.error_message << std::endl;
    }
}

void ExampleCompileFromFile() {
    FirstEngine::Shader::ShaderSourceCompiler compiler;
    
    // 自动检测文件类型和shader stage
    auto result = compiler.CompileFromFileAuto("shaders/vertex.vert");
    
    if (result.success) {
        std::cout << "✅ 文件编译成功！" << std::endl;
        
        // 保存编译结果
        FirstEngine::Shader::ShaderSourceCompiler::SaveSPIRV(result.spirv_code, "output.spv");
    } else {
        std::cerr << "❌ 文件编译失败: " << result.error_message << std::endl;
    }
}

void ExampleWithDefines() {
    // 使用宏定义编译shader
    const char* shader_src = R"(
#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 0) out vec4 fragColor;

#ifdef USE_TEXTURE
uniform sampler2D u_Texture;
#endif

void main() {
    gl_Position = vec4(inPosition, 1.0);
    #ifdef USE_TEXTURE
    fragColor = texture(u_Texture, vec2(0.5));
    #else
    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
    #endif
}
)";

    FirstEngine::Shader::ShaderSourceCompiler compiler;
    FirstEngine::Shader::CompileOptions options;
    options.stage = FirstEngine::Shader::ShaderStage::Vertex;
    options.language = FirstEngine::Shader::ShaderSourceLanguage::GLSL;
    
    // 添加宏定义
    options.defines.push_back({"USE_TEXTURE", "1"});
    options.defines.push_back({"MAX_LIGHTS", "4"});
    
    auto result = compiler.CompileGLSL(shader_src, options);
    
    if (result.success) {
        std::cout << "✅ 带宏定义的shader编译成功！" << std::endl;
    }
}

int main() {
    std::cout << "=== ShaderSourceCompiler 示例 ===" << std::endl;
    std::cout << std::endl;
    
    std::cout << "1. 编译GLSL示例:" << std::endl;
    ExampleCompileGLSL();
    std::cout << std::endl;
    
    std::cout << "2. 编译HLSL示例:" << std::endl;
    ExampleCompileHLSL();
    std::cout << std::endl;
    
    std::cout << "3. 从文件编译示例:" << std::endl;
    ExampleCompileFromFile();
    std::cout << std::endl;
    
    std::cout << "4. 带宏定义编译示例:" << std::endl;
    ExampleWithDefines();
    std::cout << std::endl;
    
    return 0;
}
