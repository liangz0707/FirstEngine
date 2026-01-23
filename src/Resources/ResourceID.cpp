#include "FirstEngine/Resources/ResourceID.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/ResourceDependency.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <limits>
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

        ResourceID ResourceIDManager::RegisterResource(const std::string& filepath, ResourceType type, const std::string& virtualPath) {
            std::string normalized = NormalizePath(filepath);
            
            // Check if already registered
            auto it = m_PathToID.find(normalized);
            if (it != m_PathToID.end()) {
                return it->second;
            }

            // Generate new ID and register
            ResourceID id = GenerateID();
            m_IDToPath[id] = filepath; // Store original path
            
            // Set virtual path (use filepath if not provided)
            std::string vpath = virtualPath.empty() ? filepath : virtualPath;
            std::string normalizedVPath = NormalizePath(vpath);
            m_IDToVirtualPath[id] = vpath;
            m_VirtualPathToID[normalizedVPath] = id;
            
            m_PathToID[normalized] = id;
            m_IDToType[id] = type;

            return id;
        }

        bool ResourceIDManager::RegisterResourceWithID(ResourceID id, const std::string& filepath, ResourceType type, const std::string& virtualPath) {
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

            // Set virtual path (use filepath if not provided)
            std::string vpath = virtualPath.empty() ? filepath : virtualPath;
            std::string normalizedVPath = NormalizePath(vpath);
            
            // Check if virtual path already registered with different ID
            if (m_VirtualPathToID.find(normalizedVPath) != m_VirtualPathToID.end()) {
                return false; // Virtual path already registered
            }

            m_IDToPath[id] = filepath;
            m_PathToID[normalized] = id;
            
            m_IDToVirtualPath[id] = vpath;
            m_VirtualPathToID[normalizedVPath] = id;
            
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
            if (it == m_IDToPath.end()) {
                std::cerr << "ResourceIDManager::GetPathFromID: Resource ID " << id << " not found in manifest." << std::endl;
                std::cerr << "  Total registered resources: " << m_IDToPath.size() << std::endl;
                if (m_IDToPath.size() > 0) {
                    // Find min and max IDs
                    ResourceID minID = std::numeric_limits<ResourceID>::max();
                    ResourceID maxID = 0;
                    for (const auto& pair : m_IDToPath) {
                        if (pair.first < minID) minID = pair.first;
                        if (pair.first > maxID) maxID = pair.first;
                    }
                    std::cerr << "  Available IDs range: " << minID << " to " << maxID << std::endl;
                }
                return std::string();
            }
            return it->second;
        }

        ResourceID ResourceIDManager::GetIDFromVirtualPath(const std::string& virtualPath) const {
            std::string normalized = NormalizePath(virtualPath);
            auto it = m_VirtualPathToID.find(normalized);
            return (it != m_VirtualPathToID.end()) ? it->second : InvalidResourceID;
        }

        std::string ResourceIDManager::GetVirtualPathFromID(ResourceID id) const {
            auto it = m_IDToVirtualPath.find(id);
            return (it != m_IDToVirtualPath.end()) ? it->second : std::string();
        }

        std::string ResourceIDManager::GetVirtualPathFromPath(const std::string& filepath) const {
            ResourceID id = GetIDFromPath(filepath);
            if (id != InvalidResourceID) {
                return GetVirtualPathFromID(id);
            }
            return std::string();
        }

        bool ResourceIDManager::IsVirtualPathRegistered(const std::string& virtualPath) const {
            std::string normalized = NormalizePath(virtualPath);
            return m_VirtualPathToID.find(normalized) != m_VirtualPathToID.end();
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
            m_IDToVirtualPath.clear();
            m_PathToID.clear();
            m_VirtualPathToID.clear();
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

        // Helper function to escape JSON string
        static std::string EscapeJSONString(const std::string& str) {
            std::ostringstream o;
            for (char c : str) {
                switch (c) {
                case '"': o << "\\\""; break;
                case '\\': o << "\\\\"; break;
                case '\b': o << "\\b"; break;
                case '\f': o << "\\f"; break;
                case '\n': o << "\\n"; break;
                case '\r': o << "\\r"; break;
                case '\t': o << "\\t"; break;
                default:
                    if ('\x00' <= c && c <= '\x1f') {
                        o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
                    } else {
                        o << c;
                    }
                }
            }
            return o.str();
        }

        // Helper function to get resource type name
        static std::string GetResourceTypeName(ResourceType type) {
            switch (type) {
            case ResourceType::Mesh: return "Mesh";
            case ResourceType::Material: return "Material";
            case ResourceType::Texture: return "Texture";
            case ResourceType::Model: return "Model";
            case ResourceType::Shader: return "Shader";
            default: return "Unknown";
            }
        }

        // Helper function to parse resource type from name
        static ResourceType ParseResourceTypeName(const std::string& name) {
            if (name == "Mesh") return ResourceType::Mesh;
            if (name == "Material") return ResourceType::Material;
            if (name == "Texture") return ResourceType::Texture;
            if (name == "Model") return ResourceType::Model;
            if (name == "Shader") return ResourceType::Shader;
            return ResourceType::Unknown;
        }

        bool ResourceIDManager::SaveManifest(const std::string& manifestPath) const {
            try {
                // Create directory if it doesn't exist
                fs::path path(manifestPath);
                if (path.has_parent_path()) {
                    fs::create_directories(path.parent_path());
                }

                std::ofstream file(manifestPath, std::ios::out | std::ios::trunc);
                if (!file.is_open()) {
                    return false;
                }

                // Write JSON header
                file << "{\n";
                file << "  \"version\": 1,\n";
                file << "  \"nextID\": " << m_NextID << ",\n";
                file << "  \"resources\": [\n";

                // Write all resource entries
                bool first = true;
                for (const auto& pair : m_IDToPath) {
                    ResourceID id = pair.first;
                    const std::string& path = pair.second;
                    
                    auto typeIt = m_IDToType.find(id);
                    ResourceType type = (typeIt != m_IDToType.end()) ? typeIt->second : ResourceType::Unknown;
                    
                    auto virtualPathIt = m_IDToVirtualPath.find(id);
                    const std::string& virtualPath = (virtualPathIt != m_IDToVirtualPath.end()) ? virtualPathIt->second : path;

                    if (!first) {
                        file << ",\n";
                    }
                    first = false;

                    file << "    {\n";
                    file << "      \"id\": " << id << ",\n";
                    file << "      \"path\": \"" << EscapeJSONString(path) << "\",\n";
                    file << "      \"virtualPath\": \"" << EscapeJSONString(virtualPath) << "\",\n";
                    file << "      \"type\": \"" << GetResourceTypeName(type) << "\"\n";
                    file << "    }";
                }

                file << "\n  ]\n";
                file << "}\n";

                file.close();
                return true;
            } catch (...) {
                return false;
            }
        }

        bool ResourceIDManager::LoadManifest(const std::string& manifestPath) {
            try {
                std::ifstream file(manifestPath, std::ios::in);
                if (!file.is_open()) {
                    std::cerr << "ResourceIDManager::LoadManifest: Failed to open manifest file: " << manifestPath << std::endl;
                    return false;
                }
                
                std::cout << "ResourceIDManager::LoadManifest: Loading manifest from: " << manifestPath << std::endl;

                // Read entire file into string
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                file.close();

                // Simple JSON parser (handles the specific format we generate)
                // This is a basic parser - for production use, consider a proper JSON library
                
                // Clear existing data
                Clear();

                // Find version and nextID
                size_t versionPos = content.find("\"version\"");
                size_t nextIDPos = content.find("\"nextID\"");
                size_t resourcesPos = content.find("\"resources\"");

                if (nextIDPos != std::string::npos) {
                    // Extract nextID value
                    size_t colonPos = content.find(':', nextIDPos);
                    if (colonPos != std::string::npos) {
                        size_t valueStart = content.find_first_not_of(" \t\n\r", colonPos + 1);
                        size_t valueEnd = content.find_first_of(",}\n\r", valueStart);
                        if (valueStart != std::string::npos && valueEnd != std::string::npos) {
                            std::string nextIDStr = content.substr(valueStart, valueEnd - valueStart);
                            try {
                                m_NextID = std::stoull(nextIDStr);
                            } catch (...) {
                                m_NextID = 1;
                            }
                        }
                    }
                }

                // Parse resources array
                if (resourcesPos != std::string::npos) {
                    size_t arrayStart = content.find('[', resourcesPos);
                    if (arrayStart != std::string::npos) {
                        size_t pos = arrayStart + 1;
                        
                        while (pos < content.length()) {
                            // Find next resource object
                            size_t objStart = content.find('{', pos);
                            if (objStart == std::string::npos) break;
                            
                            size_t objEnd = content.find('}', objStart);
                            if (objEnd == std::string::npos) break;

                            std::string objStr = content.substr(objStart, objEnd - objStart + 1);
                            
                            // Extract ID
                            size_t idPos = objStr.find("\"id\"");
                            size_t pathPos = objStr.find("\"path\"");
                            size_t virtualPathPos = objStr.find("\"virtualPath\"");
                            size_t typePos = objStr.find("\"type\"");
                            
                            if (idPos != std::string::npos && pathPos != std::string::npos && typePos != std::string::npos) {
                                // Parse ID
                                size_t idColon = objStr.find(':', idPos);
                                size_t idValueStart = objStr.find_first_not_of(" \t\n\r", idColon + 1);
                                size_t idValueEnd = objStr.find_first_of(",}\n\r", idValueStart);
                                std::string idStr = objStr.substr(idValueStart, idValueEnd - idValueStart);
                                
                                // Parse path
                                size_t pathColon = objStr.find(':', pathPos);
                                size_t pathQuoteStart = objStr.find('"', pathColon);
                                size_t pathQuoteEnd = objStr.find('"', pathQuoteStart + 1);
                                std::string pathStr = objStr.substr(pathQuoteStart + 1, pathQuoteEnd - pathQuoteStart - 1);
                                
                                // Parse virtual path (optional, for backward compatibility)
                                std::string virtualPathStr;
                                if (virtualPathPos != std::string::npos) {
                                    size_t virtualPathColon = objStr.find(':', virtualPathPos);
                                    size_t virtualPathQuoteStart = objStr.find('"', virtualPathColon);
                                    size_t virtualPathQuoteEnd = objStr.find('"', virtualPathQuoteStart + 1);
                                    if (virtualPathQuoteStart != std::string::npos && virtualPathQuoteEnd != std::string::npos) {
                                        virtualPathStr = objStr.substr(virtualPathQuoteStart + 1, virtualPathQuoteEnd - virtualPathQuoteStart - 1);
                                    }
                                }
                                
                                // Parse type
                                size_t typeColon = objStr.find(':', typePos);
                                size_t typeQuoteStart = objStr.find('"', typeColon);
                                size_t typeQuoteEnd = objStr.find('"', typeQuoteStart + 1);
                                std::string typeStr = objStr.substr(typeQuoteStart + 1, typeQuoteEnd - typeQuoteStart - 1);
                                
                                // Helper function to unescape JSON string
                                auto UnescapeJSONString = [](const std::string& str) -> std::string {
                                    std::string result;
                                    for (size_t i = 0; i < str.length(); ++i) {
                                        if (str[i] == '\\' && i + 1 < str.length()) {
                                            switch (str[i + 1]) {
                                            case '"': result += '"'; ++i; break;
                                            case '\\': result += '\\'; ++i; break;
                                            case 'n': result += '\n'; ++i; break;
                                            case 'r': result += '\r'; ++i; break;
                                            case 't': result += '\t'; ++i; break;
                                            case 'b': result += '\b'; ++i; break;
                                            case 'f': result += '\f'; ++i; break;
                                            case 'u': 
                                                // Simple Unicode escape handling (skip for now)
                                                if (i + 5 < str.length()) {
                                                    i += 5; // Skip \uXXXX
                                                }
                                                break;
                                            default: result += str[i]; break;
                                            }
                                        } else {
                                            result += str[i];
                                        }
                                    }
                                    return result;
                                };
                                
                                // Unescape paths
                                std::string unescapedPath = UnescapeJSONString(pathStr);
                                std::string unescapedVirtualPath = virtualPathStr.empty() ? unescapedPath : UnescapeJSONString(virtualPathStr);
                                
                                try {
                                    ResourceID id = std::stoull(idStr);
                                    ResourceType type = ParseResourceTypeName(typeStr);
                                    
                                    // Register the resource with virtual path
                                    if (RegisterResourceWithID(id, unescapedPath, type, unescapedVirtualPath)) {
                                        // Debug output for specific ID
                                        if (id == 4001) {
                                            std::cout << "ResourceIDManager: Registered ID 4001 -> Path: " << unescapedPath << ", Type: " << typeStr << std::endl;
                                        }
                                    }
                                } catch (const std::exception& e) {
                                    std::cerr << "ResourceIDManager: Exception parsing resource entry: " << e.what() << std::endl;
                                } catch (...) {
                                    // Skip invalid entries
                                    std::cerr << "ResourceIDManager: Unknown exception parsing resource entry" << std::endl;
                                }
                            }
                            
                            pos = objEnd + 1;
                            
                            // Check if there are more objects
                            size_t nextObj = content.find('{', pos);
                            if (nextObj == std::string::npos || nextObj > content.find(']', arrayStart)) {
                                break;
                            }
                        }
                    }
                }

                std::cout << "ResourceIDManager::LoadManifest: Successfully loaded " << m_IDToPath.size() << " resources from manifest" << std::endl;
                return true;
            } catch (const std::exception& e) {
                std::cerr << "ResourceIDManager::LoadManifest: Exception loading manifest: " << e.what() << std::endl;
                return false;
            } catch (...) {
                std::cerr << "ResourceIDManager::LoadManifest: Unknown exception loading manifest" << std::endl;
                return false;
            }
        }

    } // namespace Resources
} // namespace FirstEngine
