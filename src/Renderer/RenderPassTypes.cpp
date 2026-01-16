#include "FirstEngine/Renderer/RenderPassTypes.h"
#include <unordered_map>

namespace FirstEngine {
    namespace Renderer {

        std::string RenderPassTypeToString(RenderPassType type) {
            static const std::unordered_map<RenderPassType, std::string> typeMap = {
                {RenderPassType::Unknown, "unknown"},
                {RenderPassType::Geometry, "geometry"},
                {RenderPassType::Lighting, "lighting"},
                {RenderPassType::Forward, "forward"},
                {RenderPassType::PostProcess, "postprocess"},
                {RenderPassType::Shadow, "shadow"},
                {RenderPassType::Skybox, "skybox"},
                {RenderPassType::UI, "ui"},
                {RenderPassType::Custom, "custom"}
            };

            auto it = typeMap.find(type);
            if (it != typeMap.end()) {
                return it->second;
            }
            return "unknown";
        }

        RenderPassType StringToRenderPassType(const std::string& str) {
            static const std::unordered_map<std::string, RenderPassType> typeMap = {
                {"unknown", RenderPassType::Unknown},
                {"geometry", RenderPassType::Geometry},
                {"lighting", RenderPassType::Lighting},
                {"forward", RenderPassType::Forward},
                {"postprocess", RenderPassType::PostProcess},
                {"shadow", RenderPassType::Shadow},
                {"skybox", RenderPassType::Skybox},
                {"ui", RenderPassType::UI},
                {"custom", RenderPassType::Custom}
            };

            auto it = typeMap.find(str);
            if (it != typeMap.end()) {
                return it->second;
            }
            return RenderPassType::Unknown;
        }

        std::string FrameGraphResourceNameToString(FrameGraphResourceName name) {
            static const std::unordered_map<FrameGraphResourceName, std::string> nameMap = {
                {FrameGraphResourceName::GBufferAlbedo, "GBufferAlbedo"},
                {FrameGraphResourceName::GBufferNormal, "GBufferNormal"},
                {FrameGraphResourceName::GBufferDepth, "GBufferDepth"},
                {FrameGraphResourceName::GBufferRoughness, "GBufferRoughness"},
                {FrameGraphResourceName::GBufferMetallic, "GBufferMetallic"},
                {FrameGraphResourceName::FinalOutput, "FinalOutput"},
                {FrameGraphResourceName::HDRBuffer, "HDRBuffer"},
                {FrameGraphResourceName::ShadowMap, "ShadowMap"},
                {FrameGraphResourceName::PostProcessBuffer, "PostProcessBuffer"},
                {FrameGraphResourceName::Custom, "Custom"}
            };

            auto it = nameMap.find(name);
            if (it != nameMap.end()) {
                return it->second;
            }
            return "Custom";
        }

        FrameGraphResourceName StringToFrameGraphResourceName(const std::string& str) {
            static const std::unordered_map<std::string, FrameGraphResourceName> nameMap = {
                {"GBufferAlbedo", FrameGraphResourceName::GBufferAlbedo},
                {"GBufferNormal", FrameGraphResourceName::GBufferNormal},
                {"GBufferDepth", FrameGraphResourceName::GBufferDepth},
                {"GBufferRoughness", FrameGraphResourceName::GBufferRoughness},
                {"GBufferMetallic", FrameGraphResourceName::GBufferMetallic},
                {"FinalOutput", FrameGraphResourceName::FinalOutput},
                {"HDRBuffer", FrameGraphResourceName::HDRBuffer},
                {"ShadowMap", FrameGraphResourceName::ShadowMap},
                {"PostProcessBuffer", FrameGraphResourceName::PostProcessBuffer},
                {"Custom", FrameGraphResourceName::Custom}
            };

            auto it = nameMap.find(str);
            if (it != nameMap.end()) {
                return it->second;
            }
            return FrameGraphResourceName::Custom;
        }

    } // namespace Renderer
} // namespace FirstEngine
