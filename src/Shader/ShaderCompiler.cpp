#include "FirstEngine/Shader/ShaderCompiler.h"
#include "FirstEngine/Shader/ShaderLoader.h"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_reflect.hpp"
#include <fstream>
#include <stdexcept>
#include <memory>
#include <iostream>

namespace FirstEngine {
    namespace Shader {
        
        static std::vector<uint32_t> LoadSPIRVFromFile(const std::string& filepath) {
            std::ifstream file(filepath, std::ios::ate | std::ios::binary);
            
            if (!file.is_open()) {
                throw std::runtime_error("Failed to open SPIR-V file: " + filepath);
            }
            
            size_t fileSize = (size_t)file.tellg();
            if (fileSize % 4 != 0) {
                throw std::runtime_error("SPIR-V file size must be a multiple of 4 bytes");
            }
            
            std::vector<uint32_t> buffer(fileSize / 4);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            file.close();
            
            return buffer;
        }
        
        class ShaderCompiler::Impl {
        public:
            // Note: compiler is removed to avoid double-deletion issue
            // We use language-specific compilers directly instead
            std::unique_ptr<spirv_cross::CompilerGLSL> glsl_compiler;
            std::unique_ptr<spirv_cross::CompilerHLSL> hlsl_compiler;
            std::unique_ptr<spirv_cross::CompilerMSL> msl_compiler;
            std::unique_ptr<spirv_cross::CompilerReflection> reflect_compiler;
            
            void Initialize(std::vector<uint32_t> spirv) {
                // Validate SPIR-V code first
                if (spirv.empty() || spirv.size() < 5) {
                    throw std::runtime_error("Invalid SPIR-V: code is empty or too short");
                }
                
                // Check SPIR-V magic number (0x07230203)
                if (spirv[0] != 0x07230203) {
                    throw std::runtime_error("Invalid SPIR-V: incorrect magic number");
                }
                
                try {
                    // Initialize reflection compiler first (most likely to fail)
                    reflect_compiler = std::make_unique<spirv_cross::CompilerReflection>(spirv);
                } catch (...) {
                    // Safe error handling - don't access exception object to avoid access violation
                    throw std::runtime_error("Failed to initialize reflection compiler");
                }
                
                try {
                    // Initialize language-specific compilers
                    glsl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv);
                    hlsl_compiler = std::make_unique<spirv_cross::CompilerHLSL>(spirv);
                    msl_compiler = std::make_unique<spirv_cross::CompilerMSL>(spirv);
                } catch (...) {
                    // If language compilers fail, we can still use reflection compiler
                    // Safe error handling - don't access exception object
                    std::cerr << "Warning: Failed to initialize language compilers" << std::endl;
                }
                
                // Set defaults (only if compilers were successfully created)
                if (glsl_compiler) {
                    spirv_cross::CompilerGLSL::Options opts;
                    opts.version = 450;
                    opts.es = false;
                    glsl_compiler->set_common_options(opts);
                }
                
                if (hlsl_compiler) {
                    spirv_cross::CompilerHLSL::Options hlsl_opts;
                    hlsl_opts.shader_model = 50;
                    hlsl_compiler->set_hlsl_options(hlsl_opts);
                }
                
                if (msl_compiler) {
                    spirv_cross::CompilerMSL::Options msl_opts;
                    msl_opts.msl_version = spirv_cross::CompilerMSL::Options::make_msl_version(2, 0);
                    msl_compiler->set_msl_options(msl_opts);
                }
                
                // Note: compiler member removed to avoid double-deletion
                // Use glsl_compiler, hlsl_compiler, msl_compiler, or reflect_compiler directly
            }
        };
        
        ShaderCompiler::ShaderCompiler(const std::vector<uint32_t>& spirv)
            : m_SPIRV(spirv) {
            m_Impl = std::make_unique<Impl>();
            m_Impl->Initialize(spirv);
            m_Compiler = m_Impl->glsl_compiler.get();
        }
        
        ShaderCompiler::ShaderCompiler(const std::string& spirv_filepath)
            : ShaderCompiler(LoadSPIRVFromFile(spirv_filepath)) {
        }
        
        ShaderCompiler::~ShaderCompiler() = default;
        
        std::string ShaderCompiler::CompileToGLSL(const std::string& entry_point) {
            if (!m_Impl->glsl_compiler) {
                throw std::runtime_error("GLSL compiler not initialized");
            }
            
            // Find entry point by name or use default
            auto entry_points = m_Impl->glsl_compiler->get_entry_points_and_stages();
            spv::ExecutionModel model = spv::ExecutionModelVertex;
            bool found = false;
            for (const auto& ep : entry_points) {
                if (ep.name == entry_point) {
                    model = ep.execution_model;
                    found = true;
                    break;
                }
            }
            if (!found && !entry_points.empty()) {
                // Use first entry point if not found
                model = entry_points[0].execution_model;
            }
            
            m_Impl->glsl_compiler->set_entry_point(entry_point, model);
            return m_Impl->glsl_compiler->compile();
        }
        
        std::string ShaderCompiler::CompileToHLSL(const std::string& entry_point) {
            if (!m_Impl->hlsl_compiler) {
                throw std::runtime_error("HLSL compiler not initialized");
            }
            
            // Find entry point by name or use default
            auto entry_points = m_Impl->hlsl_compiler->get_entry_points_and_stages();
            spv::ExecutionModel model = spv::ExecutionModelVertex;
            bool found = false;
            for (const auto& ep : entry_points) {
                if (ep.name == entry_point) {
                    model = ep.execution_model;
                    found = true;
                    break;
                }
            }
            if (!found && !entry_points.empty()) {
                model = entry_points[0].execution_model;
            }
            
            m_Impl->hlsl_compiler->set_entry_point(entry_point, model);
            return m_Impl->hlsl_compiler->compile();
        }
        
        std::string ShaderCompiler::CompileToMSL(const std::string& entry_point) {
            if (!m_Impl->msl_compiler) {
                throw std::runtime_error("MSL compiler not initialized");
            }
            
            // Find entry point by name or use default
            auto entry_points = m_Impl->msl_compiler->get_entry_points_and_stages();
            spv::ExecutionModel model = spv::ExecutionModelVertex;
            bool found = false;
            for (const auto& ep : entry_points) {
                if (ep.name == entry_point) {
                    model = ep.execution_model;
                    found = true;
                    break;
                }
            }
            if (!found && !entry_points.empty()) {
                model = entry_points[0].execution_model;
            }
            
            m_Impl->msl_compiler->set_entry_point(entry_point, model);
            return m_Impl->msl_compiler->compile();
        }
        
        ShaderReflection ShaderCompiler::GetReflection() const {
            ShaderReflection reflection;
            reflection.language = ShaderLanguage::SPIRV;
            
            if (!m_Impl->reflect_compiler) {
                return reflection;
            }
            
            // Wrap entire reflection parsing in try-catch to prevent crashes
            try {
                auto& compiler = *m_Impl->reflect_compiler;
                auto resources = compiler.get_shader_resources();
                
                // IMPORTANT: If no texture resources found, check if we need to combine resources from multiple shader stages
                // For now, we'll process what we have from this shader stage
                
                // Get entry point
                auto entry_points = compiler.get_entry_points_and_stages();
                if (!entry_points.empty()) {
                    reflection.entry_point = entry_points[0].name;
                }
                
                // Get uniform buffers (from AST)
                for (auto& resource : resources.uniform_buffers) {
                try {
                    UniformBuffer ub;
                    ub.name = resource.name;
                    ub.id = resource.id;
                    
                    // Get decorations with error handling
                    try {
                        ub.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        ub.set = 0; // Default set
                    }
                    
                    try {
                        ub.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        ub.binding = 0; // Default binding
                    }
                    
                    // Get buffer size from AST
                    try {
                        auto& type = compiler.get_type(resource.base_type_id);
                        ub.size = (uint32_t)compiler.get_declared_struct_size(type);
                        
                        // Get members from AST
                        if (type.basetype == spirv_cross::SPIRType::Struct) {
                            for (uint32_t i = 0; i < type.member_types.size(); i++) {
                                try {
                                    ShaderResource member;
                                    member.name = compiler.get_member_name(resource.base_type_id, i);
                                    member.id = type.member_types[i];
                                    
                                    auto& member_type = compiler.get_type(type.member_types[i]);
                                    member.size = (uint32_t)compiler.get_declared_struct_member_size(type, i);
                                    member.base_type_id = member_type.self;
                                    
                                    // Get member offset from AST (for std140 layout)
                                    try {
                                        member.offset = compiler.get_member_decoration(resource.base_type_id, i, spv::DecorationOffset);
                                    } catch (...) {
                                        // Offset decoration not available, will calculate sequentially
                                        member.offset = 0; // Will be calculated based on previous members
                                    }
                                    
                                    // Get array size from AST (convert SmallVector to std::vector)
                                    member.array_size.assign(member_type.array.begin(), member_type.array.end());
                                    
                                    ub.members.push_back(member);
                                } catch (...) {
                                    // Skip invalid member
                                    continue;
                                }
                            }
                        }
                    } catch (...) {
                        ub.size = 0; // Failed to get size
                    }
                    
                    reflection.uniform_buffers.push_back(ub);
                } catch (...) {
                    // Skip invalid uniform buffer
                    continue;
                }
                }
                
                // Get combined image samplers (sampler2D in GLSL)
                std::cerr << "ShaderCompiler::GetReflection: Processing " << resources.sampled_images.size() << " sampled_images..." << std::endl;
                for (auto& resource : resources.sampled_images) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        sr.set = 0;
                    }
                    try {
                        sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        sr.binding = 0;
                    }
                    // Store in both samplers (for backward compatibility) and sampled_images (for type distinction)
                    reflection.samplers.push_back(sr);
                    reflection.sampled_images.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid sampler
                }
                }
                
                // Get separate images (texture2D in HLSL/Vulkan, used with separate samplers)
                for (auto& resource : resources.separate_images) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        sr.set = 0;
                    }
                    try {
                        sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        sr.binding = 0;
                    }
                    // Store in both images (for backward compatibility) and separate_images (for type distinction)
                    reflection.images.push_back(sr);
                    reflection.separate_images.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid image
                }
                }
                
                // Get separate samplers (sampler in HLSL/Vulkan, used with separate images)
                for (auto& resource : resources.separate_samplers) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        sr.set = 0;
                    }
                    try {
                        sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        sr.binding = 0;
                    }
                    // Store in both samplers (for backward compatibility) and separate_samplers (for type distinction)
                    reflection.samplers.push_back(sr);
                    reflection.separate_samplers.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid sampler
                }
                }
                
                // Get storage images (for compute shaders, image2D with write access)
                for (auto& resource : resources.storage_images) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        sr.set = 0;
                    }
                    try {
                        sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        sr.binding = 0;
                    }
                    reflection.images.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid image
                }
                }
                
                // Get storage buffers
                for (auto& resource : resources.storage_buffers) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                    } catch (...) {
                        sr.set = 0;
                    }
                    try {
                        sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                    } catch (...) {
                        sr.binding = 0;
                    }
                    
                    try {
                        auto& type = compiler.get_type(resource.base_type_id);
                        sr.size = (uint32_t)compiler.get_declared_struct_size(type);
                    } catch (...) {
                        sr.size = 0;
                    }
                
                    reflection.storage_buffers.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid storage buffer
                }
                }
                
                // Get stage inputs/outputs
                for (auto& resource : resources.stage_inputs) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        auto& type = compiler.get_type(resource.base_type_id);
                        sr.base_type_id = type.self;
                        
                        // Get location decoration
                        if (compiler.has_decoration(resource.id, spv::DecorationLocation)) {
                            sr.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
                        }
                        
                        // Get component decoration
                        if (compiler.has_decoration(resource.id, spv::DecorationComponent)) {
                            sr.component = compiler.get_decoration(resource.id, spv::DecorationComponent);
                        }
                        
                        // Get type information
                        sr.basetype = static_cast<uint32_t>(type.basetype);
                        sr.width = type.width;
                        sr.vecsize = type.vecsize;
                        sr.columns = type.columns;
                    } catch (...) {
                        // Failed to get type information, use defaults
                        sr.base_type_id = 0;
                        sr.location = 0;
                        sr.component = 0;
                        sr.basetype = 0;
                        sr.width = 0;
                        sr.vecsize = 0;
                        sr.columns = 0;
                    }
                    
                    reflection.stage_inputs.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid stage input
                }
                }
                
                for (auto& resource : resources.stage_outputs) {
                try {
                    ShaderResource sr;
                    sr.name = resource.name;
                    sr.id = resource.id;
                    try {
                        auto& type = compiler.get_type(resource.base_type_id);
                        sr.base_type_id = type.self;
                        
                        // Get location decoration
                        if (compiler.has_decoration(resource.id, spv::DecorationLocation)) {
                            sr.location = compiler.get_decoration(resource.id, spv::DecorationLocation);
                        }
                        
                        // Get component decoration
                        if (compiler.has_decoration(resource.id, spv::DecorationComponent)) {
                            sr.component = compiler.get_decoration(resource.id, spv::DecorationComponent);
                        }
                        
                        // Get type information
                        sr.basetype = static_cast<uint32_t>(type.basetype);
                        sr.width = type.width;
                        sr.vecsize = type.vecsize;
                        sr.columns = type.columns;
                    } catch (...) {
                        // Failed to get type information, use defaults
                        sr.base_type_id = 0;
                        sr.location = 0;
                        sr.component = 0;
                        sr.basetype = 0;
                        sr.width = 0;
                        sr.vecsize = 0;
                        sr.columns = 0;
                    }
                    
                    reflection.stage_outputs.push_back(sr);
                } catch (...) {
                    continue; // Skip invalid stage output
                }
                }
                
                // Get push constant size
                for (auto& resource : resources.push_constant_buffers) {
                    try {
                        auto& type = compiler.get_type(resource.base_type_id);
                        reflection.push_constant_size = (uint32_t)compiler.get_declared_struct_size(type);
                        break;
                    } catch (...) {
                        reflection.push_constant_size = 0;
                        break;
                    }
                }
                
            } catch (...) {
                // If reflection parsing fails completely, return empty reflection
                // This prevents crashes and allows shader to still be used
                // Don't access exception object to avoid access violation
            }
            
            return reflection;
        }
        
        std::vector<ShaderResource> ShaderCompiler::GetUniformBuffers() const {
            auto reflection = GetReflection();
            std::vector<ShaderResource> result;
            
            for (const auto& ub : reflection.uniform_buffers) {
                ShaderResource sr;
                sr.name = ub.name;
                sr.id = ub.id;
                sr.set = ub.set;
                sr.binding = ub.binding;
                sr.size = ub.size;
                result.push_back(sr);
            }
            
            return result;
        }
        
        std::vector<ShaderResource> ShaderCompiler::GetSamplers() const {
            return GetReflection().samplers;
        }
        
        std::vector<ShaderResource> ShaderCompiler::GetImages() const {
            return GetReflection().images;
        }
        
        std::vector<ShaderResource> ShaderCompiler::GetStorageBuffers() const {
            return GetReflection().storage_buffers;
        }
        
        void ShaderCompiler::SetGLSLVersion(uint32_t version) {
            if (!m_Impl->glsl_compiler) return;
            
            spirv_cross::CompilerGLSL::Options opts;
            opts.version = version;
            opts.es = (version < 300);
            m_Impl->glsl_compiler->set_common_options(opts);
        }
        
        void ShaderCompiler::SetHLSLShaderModel(uint32_t model) {
            if (!m_Impl->hlsl_compiler) return;
            
            spirv_cross::CompilerHLSL::Options opts;
            opts.shader_model = model;
            m_Impl->hlsl_compiler->set_hlsl_options(opts);
        }
        
        void ShaderCompiler::SetMSLVersion(uint32_t version) {
            if (!m_Impl->msl_compiler) return;
            
            spirv_cross::CompilerMSL::Options opts;
            uint32_t major = version / 10000;
            uint32_t minor = (version % 10000) / 100;
            opts.msl_version = spirv_cross::CompilerMSL::Options::make_msl_version(major, minor);
            m_Impl->msl_compiler->set_msl_options(opts);
        }
        
    }
}
