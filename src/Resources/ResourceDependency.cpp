#include "FirstEngine/Resources/ResourceDependency.h"
#include <algorithm>
#include <cctype>

#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        std::string ResourcePathResolver::ResolvePath(const std::string& path, const std::string& basePath) {
            if (path.empty()) {
                return path;
            }

            // Normalize paths
            std::string normalizedPath = NormalizePath(path);
            std::string normalizedBase = NormalizePath(basePath);

            // If path is already absolute, return it
            if (IsAbsolutePath(normalizedPath)) {
                return normalizedPath;
            }

            // If base path is empty, return the path as-is
            if (normalizedBase.empty()) {
                return normalizedPath;
            }

            // Resolve relative path
            try {
                fs::path baseDir = fs::path(normalizedBase).parent_path();
                if (baseDir.empty()) {
                    baseDir = fs::current_path();
                }
                fs::path resolved = baseDir / normalizedPath;
                resolved = fs::canonical(resolved); // Resolve .. and .
                return resolved.string();
            } catch (...) {
                // If canonical fails, just concatenate
                fs::path baseDir = fs::path(normalizedBase).parent_path();
                fs::path resolved = baseDir / normalizedPath;
                return resolved.string();
            }
        }

        std::string ResourcePathResolver::GetDirectory(const std::string& filepath) {
            try {
                fs::path path(filepath);
                return path.parent_path().string();
            } catch (...) {
                return "";
            }
        }

        std::string ResourcePathResolver::NormalizePath(const std::string& path) {
            if (path.empty()) {
                return path;
            }

            std::string normalized = path;
            
            // Convert backslashes to forward slashes (for Windows compatibility)
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            
            // Remove redundant separators
            std::string result;
            result.reserve(normalized.size());
            bool lastWasSlash = false;
            for (char c : normalized) {
                if (c == '/') {
                    if (!lastWasSlash) {
                        result += c;
                    }
                    lastWasSlash = true;
                } else {
                    result += c;
                    lastWasSlash = false;
                }
            }
            
            return result;
        }

        bool ResourcePathResolver::IsAbsolutePath(const std::string& path) {
            if (path.empty()) {
                return false;
            }

            // Windows absolute path (C:\ or \\)
            if (path.length() >= 2) {
                if (std::isalpha(path[0]) && path[1] == ':') {
                    return true; // C:/
                }
                if (path[0] == '/' && path[1] == '/') {
                    return true; // UNC path
                }
            }

            // Unix absolute path
            if (path[0] == '/') {
                return true;
            }

            return false;
        }

    } // namespace Resources
} // namespace FirstEngine
