#include "FirstEngine/Shader/ShaderLoader.h"
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace FirstEngine {
    namespace Shader {
        std::vector<uint32_t> ShaderLoader::LoadSPIRV(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filepath);
            }

            size_t fileSize = (size_t)file.tellg();
            if (fileSize % 4 != 0) {
                throw std::runtime_error("SPIR-V file size must be a multiple of 4 bytes");
            }

            std::vector<uint32_t> buffer(fileSize / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            file.close();

            return buffer;
        }
        
        std::string ShaderLoader::LoadSource(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filepath);
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            file.close();
            
            return buffer.str();
        }
        
        std::vector<char> ShaderLoader::LoadShader(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);

            if (!file.is_open()) {
                throw std::runtime_error("Failed to open shader file: " + filepath);
            }

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();

            return buffer;
        }
    }
}
