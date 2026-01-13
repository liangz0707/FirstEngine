#include "FirstEngine/ShaderManager/ShaderManager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace FirstEngine {
    namespace ShaderManager {
        
        ShaderManager::ShaderManager() = default;
        ShaderManager::~ShaderManager() = default;
        
        bool ShaderManager::ParseArguments(int argc, char* argv[]) {
            if (argc < 2) {
                m_Command = Command::Help;
                return true;
            }
            
            std::string command_str = argv[1];
            std::transform(command_str.begin(), command_str.end(), command_str.begin(), ::tolower);
            
            if (command_str == "compile" || command_str == "c") {
                m_Command = Command::Compile;
            } else if (command_str == "convert" || command_str == "conv") {
                m_Command = Command::Convert;
            } else if (command_str == "reflect" || command_str == "r") {
                m_Command = Command::Reflect;
            } else if (command_str == "help" || command_str == "h" || command_str == "-h" || command_str == "--help") {
                m_Command = Command::Help;
                return true;
            } else {
                std::cerr << "Unknown command: " << command_str << std::endl;
                m_Command = Command::Help;
                return false;
            }
            
            // Parse command-specific arguments
            for (int i = 2; i < argc; i++) {
                std::string arg = argv[i];
                
                if (arg == "-i" || arg == "--input") {
                    if (i + 1 < argc) {
                        if (m_Command == Command::Compile) {
                            m_CompileOptions.input_file = argv[++i];
                        } else {
                            m_ConvertOptions.input_file = argv[++i];
                            m_ReflectOptions.input_file = argv[i];
                        }
                    }
                } else if (arg == "-o" || arg == "--output") {
                    if (i + 1 < argc) {
                        if (m_Command == Command::Compile) {
                            m_CompileOptions.output_file = argv[++i];
                        } else {
                            m_ConvertOptions.output_file = argv[++i];
                        }
                    }
                } else if (arg == "-s" || arg == "--stage") {
                    if (i + 1 < argc && m_Command == Command::Compile) {
                        m_CompileOptions.stage = ParseStage(argv[++i]);
                    }
                } else if (arg == "-l" || arg == "--language") {
                    if (i + 1 < argc && m_Command == Command::Compile) {
                        m_CompileOptions.language = ParseLanguage(argv[++i]);
                    }
                } else if (arg == "-e" || arg == "--entry") {
                    if (i + 1 < argc) {
                        if (m_Command == Command::Compile) {
                            m_CompileOptions.entry_point = argv[++i];
                        } else {
                            m_ConvertOptions.entry_point = argv[++i];
                        }
                    }
                } else if (arg == "-O" || arg == "--optimize") {
                    if (i + 1 < argc && m_Command == Command::Compile) {
                        m_CompileOptions.optimization_level = std::stoi(argv[++i]);
                    }
                } else if (arg == "-g" || arg == "--debug") {
                    if (m_Command == Command::Compile) {
                        m_CompileOptions.generate_debug_info = true;
                    }
                } else if (arg == "-D" || arg == "--define") {
                    if (i + 1 < argc && m_Command == Command::Compile) {
                        std::string define = argv[++i];
                        size_t eq_pos = define.find('=');
                        if (eq_pos != std::string::npos) {
                            m_CompileOptions.defines.push_back({
                                define.substr(0, eq_pos),
                                define.substr(eq_pos + 1)
                            });
                        } else {
                            m_CompileOptions.defines.push_back({define, ""});
                        }
                    }
                } else if (arg == "-f" || arg == "--format") {
                    if (i + 1 < argc && m_Command == Command::Convert) {
                        m_ConvertOptions.target_format = ParseOutputFormat(argv[++i]);
                    }
                } else if (arg == "--no-resources" && m_Command == Command::Reflect) {
                    m_ReflectOptions.show_resources = false;
                } else if (arg == "--no-ub" && m_Command == Command::Reflect) {
                    m_ReflectOptions.show_uniform_buffers = false;
                } else if (arg == "--no-samplers" && m_Command == Command::Reflect) {
                    m_ReflectOptions.show_samplers = false;
                } else if (arg == "--no-images" && m_Command == Command::Reflect) {
                    m_ReflectOptions.show_images = false;
                } else if (arg == "--no-storage" && m_Command == Command::Reflect) {
                    m_ReflectOptions.show_storage_buffers = false;
                } else if (arg[0] == '-') {
                    std::cerr << "Unknown option: " << arg << std::endl;
                } else if (m_Command == Command::Compile && m_CompileOptions.input_file.empty()) {
                    m_CompileOptions.input_file = arg;
                } else if (m_Command == Command::Convert && m_ConvertOptions.input_file.empty()) {
                    m_ConvertOptions.input_file = arg;
                } else if (m_Command == Command::Reflect && m_ReflectOptions.input_file.empty()) {
                    m_ReflectOptions.input_file = arg;
                }
            }
            
            // Auto-detect options if not specified
            if (m_Command == Command::Compile && !m_CompileOptions.input_file.empty()) {
                AutoDetectOptions(m_CompileOptions);
            }
            
            return true;
        }
        
        int ShaderManager::Execute() {
            switch (m_Command) {
                case Command::Compile:
                    return ExecuteCompile();
                case Command::Convert:
                    return ExecuteConvert();
                case Command::Reflect:
                    return ExecuteReflect();
                case Command::Help:
                    PrintHelp();
                    return 0;
                default:
                    std::cerr << "No command specified" << std::endl;
                    PrintHelp();
                    return 1;
            }
        }
        
        int ShaderManager::ExecuteCompile() {
            if (m_CompileOptions.input_file.empty()) {
                std::cerr << "Error: Input file not specified" << std::endl;
                return 1;
            }
            
            if (!std::filesystem::exists(m_CompileOptions.input_file)) {
                std::cerr << "Error: Input file not found: " << m_CompileOptions.input_file << std::endl;
                return 1;
            }
            
            std::cout << "Compiling shader..." << std::endl;
            std::cout << "  Input: " << m_CompileOptions.input_file << std::endl;
            std::cout << "  Stage: " << static_cast<int>(m_CompileOptions.stage) << std::endl;
            std::cout << "  Language: " << (m_CompileOptions.language == FirstEngine::Shader::ShaderSourceLanguage::GLSL ? "GLSL" : "HLSL") << std::endl;
            std::cout << "  Entry Point: " << m_CompileOptions.entry_point << std::endl;
            
            FirstEngine::Shader::ShaderSourceCompiler compiler;
            FirstEngine::Shader::CompileOptions options;
            options.stage = m_CompileOptions.stage;
            options.language = m_CompileOptions.language;
            options.entry_point = m_CompileOptions.entry_point;
            options.optimization_level = m_CompileOptions.optimization_level;
            options.generate_debug_info = m_CompileOptions.generate_debug_info;
            options.defines = m_CompileOptions.defines;
            
            FirstEngine::Shader::CompileResult result;
            if (m_CompileOptions.language == FirstEngine::Shader::ShaderSourceLanguage::GLSL) {
                std::ifstream file(m_CompileOptions.input_file);
                std::stringstream buffer;
                buffer << file.rdbuf();
                result = compiler.CompileGLSL(buffer.str(), options);
            } else {
                std::ifstream file(m_CompileOptions.input_file);
                std::stringstream buffer;
                buffer << file.rdbuf();
                result = compiler.CompileHLSL(buffer.str(), options);
            }
            
            if (!result.success) {
                std::cerr << "Compilation failed:" << std::endl;
                std::cerr << result.error_message << std::endl;
                return 1;
            }
            
            // Print warnings
            if (!result.warnings.empty()) {
                std::cout << "Warnings:" << std::endl;
                for (const auto& warning : result.warnings) {
                    std::cout << "  " << warning << std::endl;
                }
            }
            
            // Determine output file
            std::string output_file = m_CompileOptions.output_file;
            if (output_file.empty()) {
                std::filesystem::path input_path(m_CompileOptions.input_file);
                output_file = input_path.stem().string() + ".spv";
            }
            
            // Save SPIR-V
            if (FirstEngine::Shader::ShaderSourceCompiler::SaveSPIRV(result.spirv_code, output_file)) {
                std::cout << "Success! Output: " << output_file << std::endl;
                std::cout << "  SPIR-V size: " << result.spirv_code.size() << " words" << std::endl;
                return 0;
            } else {
                std::cerr << "Error: Failed to save output file" << std::endl;
                return 1;
            }
        }
        
        int ShaderManager::ExecuteConvert() {
            if (m_ConvertOptions.input_file.empty()) {
                std::cerr << "Error: Input file not specified" << std::endl;
                return 1;
            }
            
            if (!std::filesystem::exists(m_ConvertOptions.input_file)) {
                std::cerr << "Error: Input file not found: " << m_ConvertOptions.input_file << std::endl;
                return 1;
            }
            
            std::cout << "Converting shader..." << std::endl;
            std::cout << "  Input: " << m_ConvertOptions.input_file << std::endl;
            
            try {
                FirstEngine::Shader::ShaderCompiler compiler(m_ConvertOptions.input_file);
                
                std::string output_code;
                std::string format_name;
                
                switch (m_ConvertOptions.target_format) {
                    case OutputFormat::GLSL:
                        output_code = compiler.CompileToGLSL(m_ConvertOptions.entry_point);
                        format_name = "GLSL";
                        break;
                    case OutputFormat::HLSL:
                        output_code = compiler.CompileToHLSL(m_ConvertOptions.entry_point);
                        format_name = "HLSL";
                        break;
                    case OutputFormat::MSL:
                        output_code = compiler.CompileToMSL(m_ConvertOptions.entry_point);
                        format_name = "MSL";
                        break;
                    default:
                        std::cerr << "Error: Unsupported output format" << std::endl;
                        return 1;
                }
                
                // Determine output file
                std::string output_file = m_ConvertOptions.output_file;
                if (output_file.empty()) {
                    std::filesystem::path input_path(m_ConvertOptions.input_file);
                    std::string ext;
                    switch (m_ConvertOptions.target_format) {
                        case OutputFormat::GLSL: ext = ".glsl"; break;
                        case OutputFormat::HLSL: ext = ".hlsl"; break;
                        case OutputFormat::MSL: ext = ".metal"; break;
                        default: ext = ".txt"; break;
                    }
                    output_file = input_path.stem().string() + ext;
                }
                
                // Save output
                std::ofstream file(output_file);
                file << output_code;
                file.close();
                
                std::cout << "Success! Output: " << output_file << " (" << format_name << ")" << std::endl;
                return 0;
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return 1;
            }
        }
        
        int ShaderManager::ExecuteReflect() {
            if (m_ReflectOptions.input_file.empty()) {
                std::cerr << "Error: Input file not specified" << std::endl;
                return 1;
            }
            
            if (!std::filesystem::exists(m_ReflectOptions.input_file)) {
                std::cerr << "Error: Input file not found: " << m_ReflectOptions.input_file << std::endl;
                return 1;
            }
            
            std::cout << "Reflecting shader..." << std::endl;
            std::cout << "  Input: " << m_ReflectOptions.input_file << std::endl;
            std::cout << std::endl;
            
            try {
                FirstEngine::Shader::ShaderCompiler compiler(m_ReflectOptions.input_file);
                auto reflection = compiler.GetReflection();
                
                std::cout << "=== Shader Reflection ===" << std::endl;
                std::cout << "Entry Point: " << reflection.entry_point << std::endl;
                std::cout << "Push Constant Size: " << reflection.push_constant_size << " bytes" << std::endl;
                std::cout << std::endl;
                
                if (m_ReflectOptions.show_resources) {
                    if (m_ReflectOptions.show_uniform_buffers && !reflection.uniform_buffers.empty()) {
                        std::cout << "Uniform Buffers (" << reflection.uniform_buffers.size() << "):" << std::endl;
                        for (const auto& ub : reflection.uniform_buffers) {
                            std::cout << "  - " << ub.name << std::endl;
                            std::cout << "    Set: " << ub.set << ", Binding: " << ub.binding << std::endl;
                            std::cout << "    Size: " << ub.size << " bytes" << std::endl;
                            if (!ub.members.empty()) {
                                std::cout << "    Members:" << std::endl;
                                for (const auto& member : ub.members) {
                                    std::cout << "      - " << member.name 
                                              << " (Size: " << member.size << " bytes)" << std::endl;
                                }
                            }
                        }
                        std::cout << std::endl;
                    }
                    
                    if (m_ReflectOptions.show_samplers && !reflection.samplers.empty()) {
                        std::cout << "Samplers (" << reflection.samplers.size() << "):" << std::endl;
                        for (const auto& sampler : reflection.samplers) {
                            std::cout << "  - " << sampler.name 
                                      << " (Set: " << sampler.set 
                                      << ", Binding: " << sampler.binding << ")" << std::endl;
                        }
                        std::cout << std::endl;
                    }
                    
                    if (m_ReflectOptions.show_images && !reflection.images.empty()) {
                        std::cout << "Images (" << reflection.images.size() << "):" << std::endl;
                        for (const auto& image : reflection.images) {
                            std::cout << "  - " << image.name 
                                      << " (Set: " << image.set 
                                      << ", Binding: " << image.binding << ")" << std::endl;
                        }
                        std::cout << std::endl;
                    }
                    
                    if (m_ReflectOptions.show_storage_buffers && !reflection.storage_buffers.empty()) {
                        std::cout << "Storage Buffers (" << reflection.storage_buffers.size() << "):" << std::endl;
                        for (const auto& sb : reflection.storage_buffers) {
                            std::cout << "  - " << sb.name 
                                      << " (Set: " << sb.set 
                                      << ", Binding: " << sb.binding 
                                      << ", Size: " << sb.size << " bytes)" << std::endl;
                        }
                        std::cout << std::endl;
                    }
                }
                
                return 0;
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                return 1;
            }
        }
        
        void ShaderManager::PrintHelp() const {
            std::cout << "ShaderManager - Shader Compilation and Conversion Tool" << std::endl;
            std::cout << std::endl;
            std::cout << "Usage: ShaderManager <command> [options]" << std::endl;
            std::cout << std::endl;
            std::cout << "Commands:" << std::endl;
            std::cout << "  compile, c    Compile GLSL/HLSL source to SPIR-V" << std::endl;
            std::cout << "  convert, conv Convert SPIR-V to GLSL/HLSL/MSL" << std::endl;
            std::cout << "  reflect, r    Show shader reflection information" << std::endl;
            std::cout << "  help, h       Show this help message" << std::endl;
            std::cout << std::endl;
            std::cout << "Compile Options:" << std::endl;
            std::cout << "  -i, --input <file>        Input shader source file" << std::endl;
            std::cout << "  -o, --output <file>       Output SPIR-V file (default: <input>.spv)" << std::endl;
            std::cout << "  -s, --stage <stage>       Shader stage (vertex, fragment, geometry, compute, etc.)" << std::endl;
            std::cout << "  -l, --language <lang>    Source language (glsl, hlsl)" << std::endl;
            std::cout << "  -e, --entry <name>        Entry point name (default: main)" << std::endl;
            std::cout << "  -O, --optimize <level>   Optimization level 0-2 (default: 0)" << std::endl;
            std::cout << "  -g, --debug               Generate debug information" << std::endl;
            std::cout << "  -D, --define <name[=val]> Define macro" << std::endl;
            std::cout << std::endl;
            std::cout << "Convert Options:" << std::endl;
            std::cout << "  -i, --input <file>        Input SPIR-V file" << std::endl;
            std::cout << "  -o, --output <file>       Output file (default: <input>.<ext>)" << std::endl;
            std::cout << "  -f, --format <format>     Target format (glsl, hlsl, msl)" << std::endl;
            std::cout << "  -e, --entry <name>        Entry point name (default: main)" << std::endl;
            std::cout << std::endl;
            std::cout << "Reflect Options:" << std::endl;
            std::cout << "  -i, --input <file>        Input SPIR-V file" << std::endl;
            std::cout << "  --no-resources            Hide all resources" << std::endl;
            std::cout << "  --no-ub                   Hide uniform buffers" << std::endl;
            std::cout << "  --no-samplers             Hide samplers" << std::endl;
            std::cout << "  --no-images               Hide images" << std::endl;
            std::cout << "  --no-storage              Hide storage buffers" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  ShaderManager compile -i vertex.vert -o vertex.spv" << std::endl;
            std::cout << "  ShaderManager convert -i shader.spv -f glsl -o shader.glsl" << std::endl;
            std::cout << "  ShaderManager reflect -i shader.spv" << std::endl;
            std::cout << std::endl;
        }
        
        FirstEngine::Shader::ShaderStage ShaderManager::ParseStage(const std::string& stage_str) {
            std::string lower = stage_str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            if (lower == "vertex" || lower == "vert" || lower == "vs") {
                return FirstEngine::Shader::ShaderStage::Vertex;
            } else if (lower == "fragment" || lower == "frag" || lower == "fs" || lower == "pixel" || lower == "ps") {
                return FirstEngine::Shader::ShaderStage::Fragment;
            } else if (lower == "geometry" || lower == "geom" || lower == "gs") {
                return FirstEngine::Shader::ShaderStage::Geometry;
            } else if (lower == "compute" || lower == "comp" || lower == "cs") {
                return FirstEngine::Shader::ShaderStage::Compute;
            } else if (lower == "tesscontrol" || lower == "tesc") {
                return FirstEngine::Shader::ShaderStage::TessellationControl;
            } else if (lower == "tesseval" || lower == "tese") {
                return FirstEngine::Shader::ShaderStage::TessellationEvaluation;
            }
            
            return FirstEngine::Shader::ShaderStage::Vertex;
        }
        
        FirstEngine::Shader::ShaderSourceLanguage ShaderManager::ParseLanguage(const std::string& lang_str) {
            std::string lower = lang_str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            if (lower == "hlsl") {
                return FirstEngine::Shader::ShaderSourceLanguage::HLSL;
            }
            
            return FirstEngine::Shader::ShaderSourceLanguage::GLSL;
        }
        
        OutputFormat ShaderManager::ParseOutputFormat(const std::string& format_str) {
            std::string lower = format_str;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            
            if (lower == "hlsl") {
                return OutputFormat::HLSL;
            } else if (lower == "msl" || lower == "metal") {
                return OutputFormat::MSL;
            }
            
            return OutputFormat::GLSL;
        }
        
        std::string ShaderManager::GetFileExtension(const std::string& filepath) {
            std::filesystem::path path(filepath);
            return path.extension().string();
        }
        
        void ShaderManager::AutoDetectOptions(CompileOptions& options) {
            std::string ext = GetFileExtension(options.input_file);
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
            
            // Detect shader stage from extension
            if (ext == ".vert" || ext == ".vertex" || ext == ".vs") {
                options.stage = FirstEngine::Shader::ShaderStage::Vertex;
            } else if (ext == ".frag" || ext == ".fragment" || ext == ".fs") {
                options.stage = FirstEngine::Shader::ShaderStage::Fragment;
            } else if (ext == ".geom" || ext == ".geometry" || ext == ".gs") {
                options.stage = FirstEngine::Shader::ShaderStage::Geometry;
            } else if (ext == ".comp" || ext == ".compute" || ext == ".cs") {
                options.stage = FirstEngine::Shader::ShaderStage::Compute;
            } else if (ext == ".tesc" || ext == ".tesscontrol") {
                options.stage = FirstEngine::Shader::ShaderStage::TessellationControl;
            } else if (ext == ".tese" || ext == ".tesseval") {
                options.stage = FirstEngine::Shader::ShaderStage::TessellationEvaluation;
            }
            
            // Detect language from extension
            if (ext == ".hlsl" || ext == ".fx" || ext == ".fxh") {
                options.language = FirstEngine::Shader::ShaderSourceLanguage::HLSL;
            } else if (ext == ".glsl" || ext == ".vert" || ext == ".frag" || 
                      ext == ".geom" || ext == ".comp" || ext == ".tesc" || ext == ".tese" ||
                      ext == ".vs" || ext == ".fs" || ext == ".gs" || ext == ".cs") {
                options.language = FirstEngine::Shader::ShaderSourceLanguage::GLSL;
            }
        }
        
    }
}
