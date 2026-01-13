#pragma once

#include "FirstEngine/Shader/Export.h"
#include <vector>
#include <string>
#include <vulkan/vulkan.h>

namespace FirstEngine {
    namespace Shader {
        class FE_SHADER_API ShaderLoader {
        public:
            static std::vector<char> LoadShader(const std::string& filepath);
            static VkShaderModule CreateShaderModule(VkDevice device, const std::vector<char>& code);
        };
    }
}

// Alias for backward compatibility
namespace FirstEngine {
    namespace Resources {
        using ShaderLoader = FirstEngine::Shader::ShaderLoader;
    }
}
