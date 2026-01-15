#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceID.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace FirstEngine {
    namespace Resources {

        // Resource dependency information (uses ResourceID instead of path)
        struct FE_RESOURCES_API ResourceDependency {
            enum class DependencyType : uint32_t {
                Mesh = 0,
                Material = 1,
                Texture = 2,
                Shader = 3,
                Model = 4
            };

            DependencyType type;
            ResourceID resourceID;   // Resource ID (replaces path)
            std::string slot;        // Slot name (e.g., "Albedo", "Normal" for textures, or mesh index as string)
            bool isRequired = true;  // Whether this dependency is required
            
            ResourceDependency() : resourceID(0), type(DependencyType::Mesh), isRequired(true) {}
            ResourceDependency(DependencyType t, ResourceID id, const std::string& s = "", bool req = true)
                : type(t), resourceID(id), slot(s), isRequired(req) {}
        };

        // Resource dependency resolver - resolves relative paths based on parent resource
        class FE_RESOURCES_API ResourcePathResolver {
        public:
            // Resolve resource path (handles relative paths)
            static std::string ResolvePath(const std::string& path, const std::string& basePath);
            
            // Get directory from file path
            static std::string GetDirectory(const std::string& filepath);
            
            // Normalize path (convert to forward slashes, remove redundant separators)
            static std::string NormalizePath(const std::string& path);
            
            // Check if path is absolute
            static bool IsAbsolutePath(const std::string& path);
        };

    } // namespace Resources
} // namespace FirstEngine
