#include "FirstEngine/Shader/ShaderCompiler.h"
#include "FirstEngine/Shader/ShaderLoader.h"
#include "spirv_glsl.hpp"
#include "spirv_hlsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_reflect.hpp"
#include <fstream>
#include <stdexcept>
#include <memory>

namespace FirstEngine {
    namespace Shader {
        
        // Helper function to load SPIR-V from file
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
            std::unique_ptr<spirv_cross::Compiler> compiler;
            std::unique_ptr<spirv_cross::CompilerGLSL> glsl_compiler;
            std::unique_ptr<spirv_cross::CompilerHLSL> hlsl_compiler;
            std::unique_ptr<spirv_cross::CompilerMSL> msl_compiler;
            std::unique_ptr<spirv_cross::CompilerReflection> reflect_compiler;
            
            void Initialize(std::vector<uint32_t> spirv) {
                // Initialize reflection compiler
                reflect_compiler = std::make_unique<spirv_cross::CompilerReflection>(spirv);
                
                // Initialize language-specific compilers
                glsl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(spirv);
                hlsl_compiler = std::make_unique<spirv_cross::CompilerHLSL>(spirv);
                msl_compiler = std::make_unique<spirv_cross::CompilerMSL>(spirv);
                
                // Set defaults
                spirv_cross::CompilerGLSL::Options opts;
                opts.version = 450;
                opts.es = false;
                glsl_compiler->set_common_options(opts);
                
                spirv_cross::CompilerHLSL::Options hlsl_opts;
                hlsl_opts.shader_model = 50;
                hlsl_compiler->set_hlsl_options(hlsl_opts);
                
                spirv_cross::CompilerMSL::Options msl_opts;
                msl_opts.msl_version = spirv_cross::CompilerMSL::Options::make_msl_version(2, 0);
                msl_compiler->set_msl_options(msl_opts);
                
                // Use GLSL compiler as base
                compiler = std::unique_ptr<spirv_cross::Compiler>(glsl_compiler.get());
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
            
            auto& compiler = *m_Impl->reflect_compiler;
            auto resources = compiler.get_shader_resources();
            
            // Get entry point
            auto entry_points = compiler.get_entry_points_and_stages();
            if (!entry_points.empty()) {
                reflection.entry_point = entry_points[0].name;
            }
            
            // Get uniform buffers (from AST)
            for (auto& resource : resources.uniform_buffers) {
                UniformBuffer ub;
                ub.name = resource.name;
                ub.id = resource.id;
                ub.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                ub.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                
                // Get buffer size from AST
                auto& type = compiler.get_type(resource.base_type_id);
                ub.size = (uint32_t)compiler.get_declared_struct_size(type);
                
                // Get members from AST
                if (type.basetype == spirv_cross::SPIRType::Struct) {
                    for (uint32_t i = 0; i < type.member_types.size(); i++) {
                        ShaderResource member;
                        member.name = compiler.get_member_name(resource.base_type_id, i);
                        member.id = type.member_types[i];
                        
                        auto& member_type = compiler.get_type(type.member_types[i]);
                        member.size = (uint32_t)compiler.get_declared_struct_member_size(type, i);
                        member.base_type_id = member_type.self;
                        
                        // Get array size from AST (convert SmallVector to std::vector)
                        member.array_size.assign(member_type.array.begin(), member_type.array.end());
                        
                        ub.members.push_back(member);
                    }
                }
                
                reflection.uniform_buffers.push_back(ub);
            }
            
            // Get samplers
            for (auto& resource : resources.sampled_images) {
                ShaderResource sr;
                sr.name = resource.name;
                sr.id = resource.id;
                sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                reflection.samplers.push_back(sr);
            }
            
            // Get images
            for (auto& resource : resources.storage_images) {
                ShaderResource sr;
                sr.name = resource.name;
                sr.id = resource.id;
                sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                reflection.images.push_back(sr);
            }
            
            // Get storage buffers
            for (auto& resource : resources.storage_buffers) {
                ShaderResource sr;
                sr.name = resource.name;
                sr.id = resource.id;
                sr.set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
                sr.binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
                
                auto& type = compiler.get_type(resource.base_type_id);
                sr.size = (uint32_t)compiler.get_declared_struct_size(type);
                
                reflection.storage_buffers.push_back(sr);
            }
            
            // Get stage inputs/outputs
            for (auto& resource : resources.stage_inputs) {
                ShaderResource sr;
                sr.name = resource.name;
                sr.id = resource.id;
                auto& type = compiler.get_type(resource.base_type_id);
                sr.base_type_id = type.self;
                reflection.stage_inputs.push_back(sr);
            }
            
            for (auto& resource : resources.stage_outputs) {
                ShaderResource sr;
                sr.name = resource.name;
                sr.id = resource.id;
                auto& type = compiler.get_type(resource.base_type_id);
                sr.base_type_id = type.self;
                reflection.stage_outputs.push_back(sr);
            }
            
            // Get push constant size
            for (auto& resource : resources.push_constant_buffers) {
                auto& type = compiler.get_type(resource.base_type_id);
                reflection.push_constant_size = (uint32_t)compiler.get_declared_struct_size(type);
                break;
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
