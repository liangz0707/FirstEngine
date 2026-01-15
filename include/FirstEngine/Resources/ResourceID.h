#pragma once

#include "FirstEngine/Resources/Export.h"
#include "FirstEngine/Resources/ResourceTypeEnum.h"
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace FirstEngine {
    namespace Resources {

        // Resource ID type - unique identifier for resources
        using ResourceID = uint64_t;
        constexpr ResourceID InvalidResourceID = 0;

        // Resource ID Manager - manages mapping between resource IDs and file paths
        // Handles ID allocation, path resolution, and ID lookup
        class FE_RESOURCES_API ResourceIDManager {
        public:
            ResourceIDManager();
            ~ResourceIDManager();

            // Register a resource with a path and get/assign an ID
            // If the path is already registered, returns the existing ID
            // virtualPath: optional virtual path (logical path), if empty, uses filepath as virtual path
            ResourceID RegisterResource(const std::string& filepath, ResourceType type, const std::string& virtualPath = "");

            // Register a resource with a specific ID
            // virtualPath: optional virtual path (logical path), if empty, uses filepath as virtual path
            bool RegisterResourceWithID(ResourceID id, const std::string& filepath, ResourceType type, const std::string& virtualPath = "");

            // Get resource ID from file path (returns InvalidResourceID if not found)
            ResourceID GetIDFromPath(const std::string& filepath) const;

            // Get resource ID from virtual path (returns InvalidResourceID if not found)
            ResourceID GetIDFromVirtualPath(const std::string& virtualPath) const;

            // Get file path from resource ID (returns empty string if not found)
            std::string GetPathFromID(ResourceID id) const;

            // Get virtual path from resource ID (returns empty string if not found)
            std::string GetVirtualPathFromID(ResourceID id) const;

            // Get virtual path from file path (returns empty string if not found)
            std::string GetVirtualPathFromPath(const std::string& filepath) const;

            // Check if a virtual path is registered
            bool IsVirtualPathRegistered(const std::string& virtualPath) const;

            // Get resource type from ID
            ResourceType GetTypeFromID(ResourceID id) const;

            // Check if an ID is registered
            bool IsRegistered(ResourceID id) const;

            // Check if a path is registered
            bool IsPathRegistered(const std::string& filepath) const;

            // Generate a new unique ID
            ResourceID GenerateID();

            // Load resource ID mappings from a manifest file (JSON format)
            bool LoadManifest(const std::string& manifestPath);

            // Save resource ID mappings to a manifest file (JSON format)
            bool SaveManifest(const std::string& manifestPath) const;

            // Clear all registrations
            void Clear();

            // Get all registered IDs of a specific type
            std::vector<ResourceID> GetIDsByType(ResourceType type) const;

        private:
            // Next available ID (auto-increment)
            ResourceID m_NextID;

            // ID -> Path mapping (physical file path)
            std::unordered_map<ResourceID, std::string> m_IDToPath;

            // ID -> Virtual Path mapping (logical path)
            std::unordered_map<ResourceID, std::string> m_IDToVirtualPath;

            // Path -> ID mapping (normalized physical paths)
            std::unordered_map<std::string, ResourceID> m_PathToID;

            // Virtual Path -> ID mapping (normalized virtual paths)
            std::unordered_map<std::string, ResourceID> m_VirtualPathToID;

            // ID -> Type mapping
            std::unordered_map<ResourceID, ResourceType> m_IDToType;

            // Normalize path for consistent lookup
            std::string NormalizePath(const std::string& path) const;
        };

    } // namespace Resources
} // namespace FirstEngine
