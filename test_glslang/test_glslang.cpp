#include "test_glslang.h"
#include <iostream>
#include "glslang/Include/glslang_c_interface.h"
#include "glslang/Public/ResourceLimits.h"

extern "C" {

TEST_GLSLANG_API int test_glslang_compile() {
    std::cout << "Testing glslang compilation..." << std::endl;
    
    // Initialize glslang
    if (glslang_initialize_process() == 0) {
        std::cout << "Failed to initialize glslang (returned 0 means failure)" << std::endl;
        return 1;
    }
    
    std::cout << "glslang initialized successfully" << std::endl;
    
    // Test basic shader compilation
    const char* shaderCode = R"(
        #version 450
        void main() {
            gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        }
    )";
    
    glslang_input_t input = {};
    input.language = GLSLANG_SOURCE_GLSL;
    input.stage = GLSLANG_STAGE_VERTEX;
    input.client = GLSLANG_CLIENT_VULKAN;
    input.client_version = GLSLANG_TARGET_VULKAN_1_0;
    input.target_language = GLSLANG_TARGET_SPV;
    input.target_language_version = GLSLANG_TARGET_SPV_1_0;
    input.code = shaderCode;
    input.default_version = 450;
    input.default_profile = GLSLANG_NO_PROFILE;
    input.force_default_version_and_profile = false;
    input.forward_compatible = false;
    input.messages = GLSLANG_MSG_DEFAULT_BIT;
    input.resource = nullptr;
    
    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!shader) {
        std::cout << "Failed to create shader" << std::endl;
        glslang_finalize_process();
        return 1;
    }
    
    if (glslang_shader_parse(shader, &input) == 0) {
        std::cout << "Shader parsing failed" << std::endl;
        const char* log = glslang_shader_get_info_log(shader);
        if (log) {
            std::cout << "Info log: " << log << std::endl;
        }
        glslang_shader_delete(shader);
        glslang_finalize_process();
        return 1;
    }
    
    std::cout << "Shader parsed successfully" << std::endl;
    
    // Create program and link
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    
    if (glslang_program_link(program, GLSLANG_MSG_DEFAULT_BIT) == 0) {
        std::cout << "Program linking failed" << std::endl;
        const char* log = glslang_program_get_info_log(program);
        if (log) {
            std::cout << "Info log: " << log << std::endl;
        }
        glslang_program_delete(program);
        glslang_shader_delete(shader);
        glslang_finalize_process();
        return 1;
    }
    
    std::cout << "Program linked successfully" << std::endl;
    
    // Generate SPIR-V
    glslang_program_SPIRV_generate(program, GLSLANG_STAGE_VERTEX);
    
    size_t spirvSize = glslang_program_SPIRV_get_size(program);
    std::cout << "Generated SPIR-V size: " << spirvSize << " words" << std::endl;
    
    if (spirvSize > 0) {
        std::cout << "SPIR-V generation successful!" << std::endl;
    } else {
        std::cout << "SPIR-V generation failed" << std::endl;
        const char* messages = glslang_program_SPIRV_get_messages(program);
        if (messages) {
            std::cout << "Messages: " << messages << std::endl;
        }
    }
    
    // Cleanup
    glslang_program_delete(program);
    glslang_shader_delete(shader);
    glslang_finalize_process();
    
    std::cout << "Test completed successfully!" << std::endl;
    return 0;
}

TEST_GLSLANG_API const char* test_glslang_get_version() {
    return "glslang 13.1.1 test DLL";
}

} // extern "C"
