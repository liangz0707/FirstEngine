#pragma once

#include "FirstEngine/Shader/Export.h"
#include <vector>
#include <string>

namespace FirstEngine {
    namespace Shader {
        // Shader file loading (graphics API agnostic)
        class FE_SHADER_API ShaderLoader {
        public:
            // Load shader file as binary data (for SPIR-V)
            static std::vector<uint32_t> LoadSPIRV(const std::string& filepath);
            
            // Load shader file as text (for GLSL/HLSL source)
            static std::string LoadSource(const std::string& filepath);
            
            // Legacy: Load shader as char vector (for backward compatibility)
            static std::vector<char> LoadShader(const std::string& filepath);
        };
    }
}

// Alias for backward compatibility
namespace FirstEngine {
    namespace Resources {
        using ShaderLoader = FirstEngine::Shader::ShaderLoader;
    }
}
