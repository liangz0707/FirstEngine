#pragma once

#include "FirstEngine/Core/Export.h"
#include <string>
#include <map>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace FirstEngine {
    namespace Core {

        // Simple INI file parser and writer
        class FE_CORE_API ConfigFile {
        public:
            ConfigFile() = default;
            ~ConfigFile() = default;

            // Load configuration from file
            bool LoadFromFile(const std::string& filepath);

            // Save configuration to file
            bool SaveToFile(const std::string& filepath) const;

            // Get value by key (returns empty string if not found)
            std::string GetValue(const std::string& key, const std::string& defaultValue = "") const;

            // Set value by key
            void SetValue(const std::string& key, const std::string& value);

            // Check if key exists
            bool HasKey(const std::string& key) const;

            // Get all keys
            std::vector<std::string> GetKeys() const;

            // Clear all entries
            void Clear();

        private:
            std::map<std::string, std::string> m_Values;

            // Helper to trim whitespace
            static std::string Trim(const std::string& str);
        };

    } // namespace Core
} // namespace FirstEngine
