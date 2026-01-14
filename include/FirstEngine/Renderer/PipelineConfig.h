#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <string>
#include <vector>
#include <map>

namespace FirstEngine {
    namespace Renderer {

        // Render pipeline configuration structure
        FE_RENDERER_API struct PassConfig {
            std::string name;
            std::string type; // "geometry", "lighting", "forward", "postprocess"
            std::vector<std::string> readResources;
            std::vector<std::string> writeResources;
            std::map<std::string, std::string> parameters;
        };

        FE_RENDERER_API struct ResourceConfig {
            std::string name;
            std::string type; // "texture", "buffer", "attachment"
            uint32_t width = 0;
            uint32_t height = 0;
            std::string format; // "R8G8B8A8_UNORM", "D32_SFLOAT", etc.
            uint64_t size = 0; // for buffers
        };

        FE_RENDERER_API struct PipelineConfig {
            std::string name;
            uint32_t width = 1280;
            uint32_t height = 720;
            std::vector<ResourceConfig> resources;
            std::vector<PassConfig> passes;
        };

        // Configuration file parser
        class FE_RENDERER_API PipelineConfigParser {
        public:
            static bool ParseFromFile(const std::string& filepath, PipelineConfig& config);
            static bool ParseFromString(const std::string& content, PipelineConfig& config);
            static bool SaveToFile(const std::string& filepath, const PipelineConfig& config);
        };

    } // namespace Renderer
} // namespace FirstEngine
