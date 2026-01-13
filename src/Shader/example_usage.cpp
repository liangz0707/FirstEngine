// ShaderCompiler 使用示例
// 这个文件展示了如何使用ShaderCompiler进行shader转换和AST访问

#include "FirstEngine/Shader/ShaderCompiler.h"
#include <iostream>
#include <fstream>

void ExampleShaderConversion() {
    try {
        // 1. 创建Shader编译器（从SPIR-V文件）
        FirstEngine::Shader::ShaderCompiler compiler("shaders/vertex.spv");
        
        // 2. 设置编译器选项
        compiler.SetGLSLVersion(450);       // GLSL 4.50
        compiler.SetHLSLShaderModel(50);    // HLSL Shader Model 5.0
        compiler.SetMSLVersion(20000);      // MSL 2.0
        
        // 3. 转换到不同语言
        std::string glsl_code = compiler.CompileToGLSL("main");
        std::string hlsl_code = compiler.CompileToHLSL("main");
        std::string msl_code = compiler.CompileToMSL("main");
        
        // 保存转换后的代码
        std::ofstream glsl_file("output.vert.glsl");
        glsl_file << glsl_code;
        glsl_file.close();
        
        std::ofstream hlsl_file("output.vert.hlsl");
        hlsl_file << hlsl_code;
        hlsl_file.close();
        
        std::ofstream msl_file("output.vert.metal");
        msl_file << msl_code;
        msl_file.close();
        
        std::cout << "Shader converted successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void ExampleShaderReflection() {
    try {
        // 加载shader
        FirstEngine::Shader::ShaderCompiler compiler("shaders/vertex.spv");
        
        // 获取反射信息（从AST）
        auto reflection = compiler.GetReflection();
        
        std::cout << "=== Shader Reflection ===" << std::endl;
        std::cout << "Entry Point: " << reflection.entry_point << std::endl;
        std::cout << "Push Constant Size: " << reflection.push_constant_size << " bytes" << std::endl;
        std::cout << std::endl;
        
        // 访问Uniform Buffers（从AST获取）
        std::cout << "Uniform Buffers (" << reflection.uniform_buffers.size() << "):" << std::endl;
        for (const auto& ub : reflection.uniform_buffers) {
            std::cout << "  - " << ub.name << std::endl;
            std::cout << "    Set: " << ub.set << ", Binding: " << ub.binding << std::endl;
            std::cout << "    Size: " << ub.size << " bytes" << std::endl;
            
            // 访问成员（从AST获取类型信息）
            if (!ub.members.empty()) {
                std::cout << "    Members:" << std::endl;
                for (const auto& member : ub.members) {
                    std::cout << "      - " << member.name 
                              << " (Size: " << member.size << " bytes";
                    if (!member.array_size.empty()) {
                        std::cout << ", Array: [";
                        for (size_t i = 0; i < member.array_size.size(); i++) {
                            if (i > 0) std::cout << ", ";
                            std::cout << member.array_size[i];
                        }
                        std::cout << "]";
                    }
                    std::cout << ")" << std::endl;
                }
            }
        }
        std::cout << std::endl;
        
        // 访问Samplers
        std::cout << "Samplers (" << reflection.samplers.size() << "):" << std::endl;
        for (const auto& sampler : reflection.samplers) {
            std::cout << "  - " << sampler.name 
                      << " (Set: " << sampler.set 
                      << ", Binding: " << sampler.binding << ")" << std::endl;
        }
        std::cout << std::endl;
        
        // 访问Images
        std::cout << "Images (" << reflection.images.size() << "):" << std::endl;
        for (const auto& image : reflection.images) {
            std::cout << "  - " << image.name 
                      << " (Set: " << image.set 
                      << ", Binding: " << image.binding << ")" << std::endl;
        }
        std::cout << std::endl;
        
        // 访问Storage Buffers
        std::cout << "Storage Buffers (" << reflection.storage_buffers.size() << "):" << std::endl;
        for (const auto& sb : reflection.storage_buffers) {
            std::cout << "  - " << sb.name 
                      << " (Set: " << sb.set 
                      << ", Binding: " << sb.binding 
                      << ", Size: " << sb.size << " bytes)" << std::endl;
        }
        std::cout << std::endl;
        
        // 访问Stage Inputs/Outputs
        std::cout << "Stage Inputs (" << reflection.stage_inputs.size() << "):" << std::endl;
        for (const auto& input : reflection.stage_inputs) {
            std::cout << "  - " << input.name << std::endl;
        }
        std::cout << std::endl;
        
        std::cout << "Stage Outputs (" << reflection.stage_outputs.size() << "):" << std::endl;
        for (const auto& output : reflection.stage_outputs) {
            std::cout << "  - " << output.name << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

void ExampleASTAccess() {
    try {
        FirstEngine::Shader::ShaderCompiler compiler("shaders/vertex.spv");
        
        // 使用便捷方法访问资源
        auto uniform_buffers = compiler.GetUniformBuffers();
        auto samplers = compiler.GetSamplers();
        auto images = compiler.GetImages();
        auto storage_buffers = compiler.GetStorageBuffers();
        
        std::cout << "=== AST-based Resource Access ===" << std::endl;
        std::cout << "Found " << uniform_buffers.size() << " uniform buffers" << std::endl;
        std::cout << "Found " << samplers.size() << " samplers" << std::endl;
        std::cout << "Found " << images.size() << " images" << std::endl;
        std::cout << "Found " << storage_buffers.size() << " storage buffers" << std::endl;
        
        // 高级AST访问（需要类型转换）
        // void* internal = compiler.GetInternalCompiler();
        // spirv_cross::Compiler* comp = static_cast<spirv_cross::Compiler*>(internal);
        // 可以在这里进行更底层的AST操作
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
