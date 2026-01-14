#pragma once

#include "FirstEngine/Core/Export.h"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace FirstEngine {
    namespace Core {

        // Command line argument type
        FE_CORE_API enum class ArgumentType {
            Flag,        // Flag (boolean value)
            String,      // String argument
            Integer,     // Integer argument
            Float,       // Float argument
            List         // List argument (can appear multiple times)
        };

        // Command line argument definition
        FE_CORE_API struct Argument {
            std::string name;           // Argument name (--name)
            std::string shortName;      // Short name (-n)
            std::string description;    // Description
            ArgumentType type;          // Type
            bool required;              // Whether required
            std::string defaultValue;   // Default value
        };

        // Command line parser
        class FE_CORE_API CommandLineParser {
        public:
            CommandLineParser();
            ~CommandLineParser();

            // Add argument definition
            void AddArgument(const Argument& arg);
            void AddArgument(const std::string& name, const std::string& shortName,
                           const std::string& description, ArgumentType type,
                           bool required = false, const std::string& defaultValue = "");

            // Parse command line arguments
            bool Parse(int argc, char* argv[]);
            bool Parse(const std::vector<std::string>& args);

            // Get argument values
            bool HasArgument(const std::string& name) const;
            bool GetBool(const std::string& name, bool defaultValue = false) const;
            std::string GetString(const std::string& name, const std::string& defaultValue = "") const;
            int GetInt(const std::string& name, int defaultValue = 0) const;
            float GetFloat(const std::string& name, float defaultValue = 0.0f) const;
            std::vector<std::string> GetList(const std::string& name) const;

            // Get positional arguments (non-option arguments)
            std::vector<std::string> GetPositionalArgs() const;

            // Show help message
            void PrintHelp(const std::string& programName) const;

            // Check if valid
            bool IsValid() const { return m_Valid; }
            std::string GetError() const { return m_Error; }

        private:
            std::vector<Argument> m_Arguments;
            std::map<std::string, std::string> m_Values;
            std::map<std::string, std::vector<std::string>> m_Lists;
            std::vector<std::string> m_PositionalArgs;
            bool m_Valid;
            mutable std::string m_Error;

            Argument* FindArgument(const std::string& name);
            const Argument* FindArgument(const std::string& name) const;
            bool ValidateArguments() const;
        };

    } // namespace Core
} // namespace FirstEngine
