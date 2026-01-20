#include "FirstEngine/Renderer/PipelineConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace FirstEngine {
    namespace Renderer {

        // Simple configuration file format (similar to JSON, but simpler)
        // Format example:
        // pipeline {
        //   name "DeferredRendering"
        //   width 1280
        //   height 720
        //   resource {
        //     name "GBufferAlbedo"
        //     type "texture"
        //     width 1280
        //     height 720
        //     format "R8G8B8A8_UNORM"
        //   }
        //   pass {
        //     name "GeometryPass"
        //     type "geometry"
        //     write "GBufferAlbedo"
        //     write "GBufferNormal"
        //     write "GBufferDepth"
        //   }
        // }

        static void SkipWhitespace(const std::string& str, size_t& pos) {
            while (pos < str.length() && (str[pos] == ' ' || str[pos] == '\t' || str[pos] == '\n' || str[pos] == '\r')) {
                pos++;
            }
        }

        static std::string ReadString(const std::string& str, size_t& pos) {
            SkipWhitespace(str, pos);
            std::string result;
            
            if (pos < str.length() && str[pos] == '"') {
                pos++; // skip opening quote
                while (pos < str.length() && str[pos] != '"') {
                    if (str[pos] == '\\' && pos + 1 < str.length()) {
                        pos++;
                        if (str[pos] == 'n') result += '\n';
                        else if (str[pos] == 't') result += '\t';
                        else if (str[pos] == '\\') result += '\\';
                        else if (str[pos] == '"') result += '"';
                        else result += str[pos];
                    } else {
                        result += str[pos];
                    }
                    pos++;
                }
                if (pos < str.length() && str[pos] == '"') pos++; // skip closing quote
            } else {
                while (pos < str.length() && str[pos] != ' ' && str[pos] != '\t' && 
                       str[pos] != '\n' && str[pos] != '\r' && str[pos] != '{' && str[pos] != '}') {
                    result += str[pos];
                    pos++;
                }
            }
            
            return result;
        }

        static uint32_t ReadUint32(const std::string& str, size_t& pos) {
            SkipWhitespace(str, pos);
            std::string numStr;
            while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') {
                numStr += str[pos];
                pos++;
            }
            return static_cast<uint32_t>(std::stoul(numStr));
        }

        static uint64_t ReadUint64(const std::string& str, size_t& pos) {
            SkipWhitespace(str, pos);
            std::string numStr;
            while (pos < str.length() && str[pos] >= '0' && str[pos] <= '9') {
                numStr += str[pos];
                pos++;
            }
            return std::stoull(numStr);
        }

        static void ParseResource(const std::string& content, size_t& pos, ResourceConfig& resource) {
            SkipWhitespace(content, pos);
            if (pos >= content.length() || content[pos] != '{') {
                throw std::runtime_error("Expected '{' for resource");
            }
            pos++; // skip '{'
            
            while (pos < content.length()) {
                SkipWhitespace(content, pos);
                if (content[pos] == '}') {
                    pos++;
                    break;
                }
                
                std::string key = ReadString(content, pos);
                SkipWhitespace(content, pos);
                
                if (key == "name") {
                    resource.name = ReadString(content, pos);
                } else if (key == "type") {
                    resource.type = ReadString(content, pos);
                } else if (key == "width") {
                    resource.width = ReadUint32(content, pos);
                } else if (key == "height") {
                    resource.height = ReadUint32(content, pos);
                } else if (key == "format") {
                    resource.format = ReadString(content, pos);
                } else if (key == "size") {
                    resource.size = ReadUint64(content, pos);
                }
            }
        }

        static void ParsePass(const std::string& content, size_t& pos, PassConfig& pass) {
            SkipWhitespace(content, pos);
            if (pos >= content.length() || content[pos] != '{') {
                throw std::runtime_error("Expected '{' for pass");
            }
            pos++; // skip '{'
            
            while (pos < content.length()) {
                SkipWhitespace(content, pos);
                if (content[pos] == '}') {
                    pos++;
                    break;
                }
                
                std::string key = ReadString(content, pos);
                SkipWhitespace(content, pos);
                
                if (key == "name") {
                    pass.name = ReadString(content, pos);
                } else if (key == "type") {
                    pass.type = ReadString(content, pos);
                } else if (key == "read") {
                    pass.readResources.push_back(ReadString(content, pos));
                } else if (key == "write") {
                    std::string writeRes = ReadString(content, pos);
                    // Debug: Check for suspicious resource names
                    if (writeRes.length() == 1 && pass.writeResources.size() > 100) {
                        std::cerr << "Warning: Suspicious write resource name in pass '" 
                                  << pass.name << "': '" << writeRes 
                                  << "' (single character). Current count: " 
                                  << pass.writeResources.size() << std::endl;
                    }
                    pass.writeResources.push_back(writeRes);
                } else {
                    std::string value = ReadString(content, pos);
                    pass.parameters[key] = value;
                }
            }
        }

        static void ParsePipeline(const std::string& content, size_t& pos, PipelineConfig& config) {
            SkipWhitespace(content, pos);
            if (pos >= content.length() || content[pos] != '{') {
                throw std::runtime_error("Expected '{' for pipeline");
            }
            pos++; // skip '{'
            
            while (pos < content.length()) {
                SkipWhitespace(content, pos);
                if (content[pos] == '}') {
                    pos++;
                    break;
                }
                
                std::string key = ReadString(content, pos);
                SkipWhitespace(content, pos);
                
                if (key == "name") {
                    config.name = ReadString(content, pos);
                } else if (key == "width") {
                    config.width = ReadUint32(content, pos);
                } else if (key == "height") {
                    config.height = ReadUint32(content, pos);
                } else if (key == "resource") {
                    ResourceConfig resource;
                    ParseResource(content, pos, resource);
                    config.resources.push_back(resource);
                } else if (key == "pass") {
                    PassConfig pass;
                    ParsePass(content, pos, pass);
                    config.passes.push_back(pass);
                }
            }
        }

        bool PipelineConfigParser::ParseFromFile(const std::string& filepath, PipelineConfig& config) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                return false;
            }
            
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            file.close();
            
            return ParseFromString(content, config);
        }

        bool PipelineConfigParser::ParseFromString(const std::string& content, PipelineConfig& config) {
            try {
                size_t pos = 0;
                SkipWhitespace(content, pos);
                
                std::string keyword = ReadString(content, pos);
                if (keyword != "pipeline") {
                    return false;
                }
                
                ParsePipeline(content, pos, config);
                return true;
            } catch (...) {
                return false;
            }
        }

        bool PipelineConfigParser::SaveToFile(const std::string& filepath, const PipelineConfig& config) {
            std::ofstream file(filepath);
            if (!file.is_open()) {
                return false;
            }
            
            file << "pipeline {\n";
            file << "  name \"" << config.name << "\"\n";
            file << "  width " << config.width << "\n";
            file << "  height " << config.height << "\n";
            
            for (const auto& resource : config.resources) {
                file << "  resource {\n";
                file << "    name \"" << resource.name << "\"\n";
                file << "    type \"" << resource.type << "\"\n";
                if (resource.type == "texture" || resource.type == "attachment") {
                    file << "    width " << resource.width << "\n";
                    file << "    height " << resource.height << "\n";
                    file << "    format \"" << resource.format << "\"\n";
                } else if (resource.type == "buffer") {
                    file << "    size " << resource.size << "\n";
                }
                file << "  }\n";
            }
            
            for (const auto& pass : config.passes) {
                file << "  pass {\n";
                file << "    name \"" << pass.name << "\"\n";
                file << "    type \"" << pass.type << "\"\n";
                for (const auto& read : pass.readResources) {
                    file << "    read \"" << read << "\"\n";
                }
                for (const auto& write : pass.writeResources) {
                    file << "    write \"" << write << "\"\n";
                }
                for (const auto& param : pass.parameters) {
                    file << "    " << param.first << " \"" << param.second << "\"\n";
                }
                file << "  }\n";
            }
            
            file << "}\n";
            file.close();
            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
