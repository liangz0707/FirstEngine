#include "FirstEngine/Core/CommandLine.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iomanip>

namespace FirstEngine {
    namespace Core {

        CommandLineParser::CommandLineParser() : m_Valid(false) {
        }

        CommandLineParser::~CommandLineParser() {
        }

        void CommandLineParser::AddArgument(const Argument& arg) {
            m_Arguments.push_back(arg);
        }

        void CommandLineParser::AddArgument(const std::string& name, const std::string& shortName,
                                           const std::string& description, ArgumentType type,
                                           bool required, const std::string& defaultValue) {
            Argument arg;
            arg.name = name;
            arg.shortName = shortName;
            arg.description = description;
            arg.type = type;
            arg.required = required;
            arg.defaultValue = defaultValue;
            m_Arguments.push_back(arg);
        }

        bool CommandLineParser::Parse(int argc, char* argv[]) {
            std::vector<std::string> args;
            for (int i = 1; i < argc; ++i) {
                args.push_back(argv[i]);
            }
            return Parse(args);
        }

        bool CommandLineParser::Parse(const std::vector<std::string>& args) {
            m_Values.clear();
            m_Lists.clear();
            m_PositionalArgs.clear();
            m_Valid = false;
            m_Error.clear();

            
            for (const auto& arg : m_Arguments) {
                if (!arg.defaultValue.empty()) {
                    m_Values[arg.name] = arg.defaultValue;
                }
            }

            
            for (size_t i = 0; i < args.size(); ++i) {
                const std::string& arg = args[i];

                if (arg.empty()) {
                    continue;
                }

                // 检查是否是选项（以-或--开头）
                if (arg[0] == '-') {
                    std::string optionName;
                    if (arg[1] == '-') {
                        // 长选项 --name
                        optionName = arg.substr(2);
                    } else {
                        // 短选项 -n
                        optionName = arg.substr(1);
                    }

                    // 查找参数定义
                    Argument* argDef = FindArgument(optionName);
                    if (!argDef) {
                        m_Error = "Unknown option: " + arg;
                        return false;
                    }

                    // 根据类型处理
                    if (argDef->type == ArgumentType::Flag) {
                        // 标志，设置为true
                        m_Values[argDef->name] = "true";
                    } else {
                        // 需要值
                        if (i + 1 >= args.size()) {
                            m_Error = "Option " + arg + " requires a value";
                            return false;
                        }
                        std::string value = args[++i];

                        if (argDef->type == ArgumentType::List) {
                            // 列表类型，可以多次出现
                            m_Lists[argDef->name].push_back(value);
                        } else {
                            // 单个值
                            m_Values[argDef->name] = value;
                        }
                    }
                } else {
                    // 位置参数
                    m_PositionalArgs.push_back(arg);
                }
            }

            // 验证必需参数
            if (!ValidateArguments()) {
                return false;
            }

            m_Valid = true;
            return true;
        }

        bool CommandLineParser::HasArgument(const std::string& name) const {
            return m_Values.find(name) != m_Values.end() || 
                   m_Lists.find(name) != m_Lists.end();
        }

        bool CommandLineParser::GetBool(const std::string& name, bool defaultValue) const {
            auto it = m_Values.find(name);
            if (it == m_Values.end()) {
                return defaultValue;
            }
            std::string value = it->second;
            std::transform(value.begin(), value.end(), value.begin(), ::tolower);
            return value == "true" || value == "1" || value == "yes" || value == "on";
        }

        std::string CommandLineParser::GetString(const std::string& name, const std::string& defaultValue) const {
            auto it = m_Values.find(name);
            if (it == m_Values.end()) {
                return defaultValue;
            }
            return it->second;
        }

        int CommandLineParser::GetInt(const std::string& name, int defaultValue) const {
            auto it = m_Values.find(name);
            if (it == m_Values.end()) {
                return defaultValue;
            }
            try {
                return std::stoi(it->second);
            } catch (...) {
                return defaultValue;
            }
        }

        float CommandLineParser::GetFloat(const std::string& name, float defaultValue) const {
            auto it = m_Values.find(name);
            if (it == m_Values.end()) {
                return defaultValue;
            }
            try {
                return std::stof(it->second);
            } catch (...) {
                return defaultValue;
            }
        }

        std::vector<std::string> CommandLineParser::GetList(const std::string& name) const {
            auto it = m_Lists.find(name);
            if (it == m_Lists.end()) {
                return std::vector<std::string>();
            }
            return it->second;
        }

        std::vector<std::string> CommandLineParser::GetPositionalArgs() const {
            return m_PositionalArgs;
        }

        void CommandLineParser::PrintHelp(const std::string& programName) const {
            std::cout << "Usage: " << programName << " [OPTIONS] [ARGS...]\n\n";
            std::cout << "Options:\n";

            for (const auto& arg : m_Arguments) {
                std::cout << "  ";
                
                // 显示选项
                if (!arg.shortName.empty()) {
                    std::cout << "-" << arg.shortName;
                    if (!arg.name.empty() && arg.name != arg.shortName) {
                        std::cout << ", ";
                    }
                }
                if (!arg.name.empty() && arg.name != arg.shortName) {
                    std::cout << "--" << arg.name;
                }

                // 显示类型
                std::cout << " ";
                switch (arg.type) {
                    case ArgumentType::Flag:
                        break;
                    case ArgumentType::String:
                        std::cout << "<string>";
                        break;
                    case ArgumentType::Integer:
                        std::cout << "<int>";
                        break;
                    case ArgumentType::Float:
                        std::cout << "<float>";
                        break;
                    case ArgumentType::List:
                        std::cout << "<value>...";
                        break;
                }

                // 显示必需标记
                if (arg.required) {
                    std::cout << " (required)";
                }

                // 显示默认值
                if (!arg.defaultValue.empty()) {
                    std::cout << " (default: " << arg.defaultValue << ")";
                }

                std::cout << "\n";

                // 显示描述
                if (!arg.description.empty()) {
                    std::cout << "      " << arg.description << "\n";
                }
                std::cout << "\n";
            }
        }

        Argument* CommandLineParser::FindArgument(const std::string& name) {
            for (auto& arg : m_Arguments) {
                if (arg.name == name || arg.shortName == name) {
                    return &arg;
                }
            }
            return nullptr;
        }

        const Argument* CommandLineParser::FindArgument(const std::string& name) const {
            for (const auto& arg : m_Arguments) {
                if (arg.name == name || arg.shortName == name) {
                    return &arg;
                }
            }
            return nullptr;
        }

        bool CommandLineParser::ValidateArguments() const {
            for (const auto& arg : m_Arguments) {
                if (arg.required) {
                    if (!HasArgument(arg.name)) {
                        m_Error = "Required argument --" + arg.name + " is missing";
                        return false;
                    }
                }
            }
            return true;
        }

    } // namespace Core
} // namespace FirstEngine
