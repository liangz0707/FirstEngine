#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        ResourceIDManager::ResourceIDManager() : m_NextID(1) {
        }

        ResourceIDManager::~ResourceIDManager() = default;

        std::string ResourceIDManager::NormalizePath(const std::string& path) const {
            std::string normalized = ResourcePathResolver::NormalizePath(path);
            // Convert to lowercase for case-insensitive comparison on Windows
            std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
            return normalized;
        }

        ResourceID ResourceIDManager::GenerateID() {
            return m_NextID++;
        }

        ResourceID ResourceIDManager::RegisterResource(const std::string& filepath, ResourceType type) {
            std::string normalized = NormalizePath(filepath);
            
            // Check if already registered
            auto it = m_PathToID.find(normalized);
            if (it != m_PathToID.end()) {
                return it->second;
            }

            // Generate new ID and register
            ResourceID id = GenerateID();
            m_IDToPath[id] = filepath; // Store original path
            m_PathToID[normalized] = id;
            m_IDToType[id] = type;

            return id;
        }

        bool ResourceIDManager::RegisterResourceWithID(ResourceID id, const std::string& filepath, ResourceType type) {
            if (id == InvalidResourceID) {
                return false;
            }

            // Check if ID already exists
            if (m_IDToPath.find(id) != m_IDToPath.end()) {
                return false; // ID already registered
            }

            std::string normalized = NormalizePath(filepath);
            
            // Check if path already registered with different ID
            if (m_PathToID.find(normalized) != m_PathToID.end()) {
                return false; // Path already registered
            }

            m_IDToPath[id] = filepath;
            m_PathToID[normalized] = id;
            m_IDToType[id] = type;

            // Update next ID if necessary
            if (id >= m_NextID) {
                m_NextID = id + 1;
            }

            return true;
        }

        ResourceID ResourceIDManager::GetIDFromPath(const std::string& filepath) const {
            std::string normalized = NormalizePath(filepath);
            auto it = m_PathToID.find(normalized);
            return (it != m_PathToID.end()) ? it->second : InvalidResourceID;
        }

        std::string ResourceIDManager::GetPathFromID(ResourceID id) const {
            auto it = m_IDToPath.find(id);
            return (it != m_IDToPath.end()) ? it->second : std::string();
        }

        ResourceType ResourceIDManager::GetTypeFromID(ResourceID id) const {
            auto it = m_IDToType.find(id);
            return (it != m_IDToType.end()) ? it->second : ResourceType::Unknown;
        }

        bool ResourceIDManager::IsRegistered(ResourceID id) const {
            return m_IDToPath.find(id) != m_IDToPath.end();
        }

        bool ResourceIDManager::IsPathRegistered(const std::string& filepath) const {
            std::string normalized = NormalizePath(filepath);
            return m_PathToID.find(normalized) != m_PathToID.end();
        }

        void ResourceIDManager::Clear() {
            m_IDToPath.clear();
            m_PathToID.clear();
            m_IDToType.clear();
            m_NextID = 1;
        }

        std::vector<ResourceID> ResourceIDManager::GetIDsByType(ResourceType type) const {
            std::vector<ResourceID> result;
            for (const auto& pair : m_IDToType) {
                if (pair.second == type) {
                    result.push_back(pair.first);
                }
            }
            return result;
        }

        bool ResourceIDManager::LoadManifest(const std::string& manifestPath) {
            // TODO: Implement JSON manifest loading
            // For now, return true (placeholder)
            (void)manifestPath;
            return true;
        }

        bool ResourceIDManager::SaveManifest(const std::string& manifestPath) const {
            // TODO: Implement JSON manifest saving
            // For now, return true (placeholder)
            (void)manifestPath;
            return true;
        }

    } // namespace Resources
} // namespace FirstEngine
