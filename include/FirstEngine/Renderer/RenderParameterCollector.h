#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Core/MathTypes.h"
#include "FirstEngine/RHI/IImage.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

// Forward declarations
namespace FirstEngine {
    namespace Resources {
        class ModelComponent;
        class MaterialResource;
        class Scene;
    }
    namespace Renderer {
        class RenderConfig;
        class IRenderPass;
    }
    namespace RHI {
        class IImage;
    }
}

namespace FirstEngine {
    namespace Renderer {

        // ============================================================================
        // RenderParameterCollector - collects render parameters from multiple sources
        // ============================================================================
        // This class collects render parameters (buffers, images, uniforms) from:
        // - ModelComponent (per-object data)
        // - RenderConfig (per-frame global data)
        // - MaterialResource (material-specific data)
        // - Camera (view/projection matrices)
        // - Scene (scene-level data)
        // - IRenderPass (pass-specific flags and config)
        // 
        // The collected parameters are then passed to ShadingMaterial for rendering
        // ============================================================================

        // Parameter value type - supports various data types
        struct FE_RENDERER_API RenderParameterValue {
            enum class Type : uint32_t {
                Texture = 0,      // RHI::IImage*
                Float = 1,
                Vec2 = 2,
                Vec3 = 3,
                Vec4 = 4,
                Int = 5,
                Bool = 6,
                Mat3 = 7,
                Mat4 = 8,
                RawData = 9,      // Raw byte data for uniform buffers
                PushConstant = 10 // Raw byte data for push constants (with offset)
            };

            Type type;
            std::vector<uint8_t> data; // Raw data storage
            uint32_t offset = 0; // Offset for push constants

            RenderParameterValue() : type(Type::Float), data(sizeof(float)), offset(0) {}
            RenderParameterValue(RHI::IImage* texture);
            RenderParameterValue(float value);
            RenderParameterValue(const Core::Vec2& value);
            RenderParameterValue(const Core::Vec3& value);
            RenderParameterValue(const Core::Vec4& value);
            RenderParameterValue(int32_t value);
            RenderParameterValue(bool value);
            RenderParameterValue(const Core::Mat3& value);
            RenderParameterValue(const Core::Mat4& value);
            RenderParameterValue(const void* rawData, uint32_t size);
            RenderParameterValue(const void* rawData, uint32_t size, uint32_t offset);

            // Getters
            RHI::IImage* GetTexture() const;
            uint32_t GetOffset() const { return offset; }
            float GetFloat() const;
            Core::Vec2 GetVec2() const;
            Core::Vec3 GetVec3() const;
            Core::Vec4 GetVec4() const;
            int32_t GetInt() const;
            bool GetBool() const;
            Core::Mat3 GetMat3() const;
            Core::Mat4 GetMat4() const;
            const void* GetRawData() const { return data.data(); }
            uint32_t GetRawDataSize() const { return static_cast<uint32_t>(data.size()); }
        };

        // Render parameters - key-value pairs
        using RenderParameters = std::unordered_map<std::string, RenderParameterValue>;

        // ============================================================================
        // RenderParameterCollector - collects parameters from multiple sources
        // ============================================================================
        class FE_RENDERER_API RenderParameterCollector {
        public:
            RenderParameterCollector();
            ~RenderParameterCollector();

            // Collect parameters from various sources
            // These methods collect parameters and store them internally
            // Parameters are merged (later sources override earlier ones)

            // Collect from ModelComponent (per-object data)
            // component: ModelComponent to collect from
            void CollectFromComponent(Resources::ModelComponent* component);

            // Collect from RenderConfig (per-frame global data)
            // config: RenderConfig to collect from
            void CollectFromRenderConfig(const RenderConfig* config);

            // Collect from MaterialResource (material-specific data)
            // materialResource: MaterialResource to collect from
            void CollectFromMaterialResource(Resources::MaterialResource* materialResource);

            // Collect from Camera (view/projection matrices)
            // viewMatrix: Camera view matrix
            // projMatrix: Camera projection matrix
            // viewProjMatrix: Combined view-projection matrix
            void CollectFromCamera(
                const Core::Mat4& viewMatrix,
                const Core::Mat4& projMatrix,
                const Core::Mat4& viewProjMatrix
            );

            // Collect from Scene (scene-level data)
            // scene: Scene to collect from
            void CollectFromScene(Resources::Scene* scene);

            // Collect from IRenderPass (pass-specific flags and config)
            // pass: IRenderPass to collect from
            void CollectFromRenderPass(IRenderPass* pass);

            // Set parameter directly (for custom parameters)
            void SetParameter(const std::string& key, const RenderParameterValue& value);

            // Get collected parameters
            const RenderParameters& GetParameters() const { return m_Parameters; }

            // Clear all collected parameters
            void Clear();

            // Merge parameters from another collector
            // Parameters from 'other' will override existing ones with the same key
            void Merge(const RenderParameterCollector& other);

        private:
            RenderParameters m_Parameters;

            // Helper methods for collecting specific parameter types
            void CollectMatrices(const Core::Mat4& view, const Core::Mat4& proj, const Core::Mat4& viewProj);
            void CollectLightData(Resources::Scene* scene);
            void CollectPassFlags(IRenderPass* pass);
        };

    } // namespace Renderer
} // namespace FirstEngine
