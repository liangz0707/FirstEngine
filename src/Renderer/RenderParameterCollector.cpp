#include "FirstEngine/Renderer/RenderParameterCollector.h"
#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/IRenderPass.h"
#include "FirstEngine/RHI/IImage.h"
#include <cstring>

namespace FirstEngine {
    namespace Renderer {

        // ============================================================================
        // RenderParameterValue implementation
        // ============================================================================

        RenderParameterValue::RenderParameterValue(RHI::IImage* texture) : type(Type::Texture), offset(0) {
            data.resize(sizeof(RHI::IImage*));
            std::memcpy(data.data(), &texture, sizeof(RHI::IImage*));
        }

        RenderParameterValue::RenderParameterValue(float value) : type(Type::Float), offset(0) {
            data.resize(sizeof(float));
            std::memcpy(data.data(), &value, sizeof(float));
        }

        RenderParameterValue::RenderParameterValue(const Core::Vec2& value) : type(Type::Vec2), offset(0) {
            data.resize(sizeof(Core::Vec2));
            std::memcpy(data.data(), &value, sizeof(Core::Vec2));
        }

        RenderParameterValue::RenderParameterValue(const Core::Vec3& value) : type(Type::Vec3), offset(0) {
            data.resize(sizeof(Core::Vec3));
            std::memcpy(data.data(), &value, sizeof(Core::Vec3));
        }

        RenderParameterValue::RenderParameterValue(const Core::Vec4& value) : type(Type::Vec4), offset(0) {
            data.resize(sizeof(Core::Vec4));
            std::memcpy(data.data(), &value, sizeof(Core::Vec4));
        }

        RenderParameterValue::RenderParameterValue(int32_t value) : type(Type::Int), offset(0) {
            data.resize(sizeof(int32_t));
            std::memcpy(data.data(), &value, sizeof(int32_t));
        }

        RenderParameterValue::RenderParameterValue(bool value) : type(Type::Bool), offset(0) {
            data.resize(sizeof(bool));
            std::memcpy(data.data(), &value, sizeof(bool));
        }

        RenderParameterValue::RenderParameterValue(const Core::Mat3& value) : type(Type::Mat3), offset(0) {
            data.resize(sizeof(Core::Mat3));
            std::memcpy(data.data(), &value, sizeof(Core::Mat3));
        }

        RenderParameterValue::RenderParameterValue(const Core::Mat4& value) : type(Type::Mat4), offset(0) {
            data.resize(sizeof(Core::Mat4));
            std::memcpy(data.data(), &value, sizeof(Core::Mat4));
        }

        RenderParameterValue::RenderParameterValue(const void* rawData, uint32_t size) : type(Type::RawData), offset(0) {
            data.resize(size);
            std::memcpy(data.data(), rawData, size);
        }

        RenderParameterValue::RenderParameterValue(const void* rawData, uint32_t size, uint32_t offset) : type(Type::PushConstant), offset(offset) {
            data.resize(size);
            std::memcpy(data.data(), rawData, size);
        }

        RHI::IImage* RenderParameterValue::GetTexture() const {
            if (type == Type::Texture && data.size() >= sizeof(RHI::IImage*)) {
                RHI::IImage* texture = nullptr;
                std::memcpy(&texture, data.data(), sizeof(RHI::IImage*));
                return texture;
            }
            return nullptr;
        }

        float RenderParameterValue::GetFloat() const {
            if (type == Type::Float && data.size() >= sizeof(float)) {
                float value = 0.0f;
                std::memcpy(&value, data.data(), sizeof(float));
                return value;
            }
            return 0.0f;
        }

        Core::Vec2 RenderParameterValue::GetVec2() const {
            if (type == Type::Vec2 && data.size() >= sizeof(Core::Vec2)) {
                Core::Vec2 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec2));
                return value;
            }
            return Core::Vec2(0.0f);
        }

        Core::Vec3 RenderParameterValue::GetVec3() const {
            if (type == Type::Vec3 && data.size() >= sizeof(Core::Vec3)) {
                Core::Vec3 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec3));
                return value;
            }
            return Core::Vec3(0.0f);
        }

        Core::Vec4 RenderParameterValue::GetVec4() const {
            if (type == Type::Vec4 && data.size() >= sizeof(Core::Vec4)) {
                Core::Vec4 value;
                std::memcpy(&value, data.data(), sizeof(Core::Vec4));
                return value;
            }
            return Core::Vec4(0.0f);
        }

        int32_t RenderParameterValue::GetInt() const {
            if (type == Type::Int && data.size() >= sizeof(int32_t)) {
                int32_t value = 0;
                std::memcpy(&value, data.data(), sizeof(int32_t));
                return value;
            }
            return 0;
        }

        bool RenderParameterValue::GetBool() const {
            if (type == Type::Bool && data.size() >= sizeof(bool)) {
                bool value = false;
                std::memcpy(&value, data.data(), sizeof(bool));
                return value;
            }
            return false;
        }

        Core::Mat3 RenderParameterValue::GetMat3() const {
            if (type == Type::Mat3 && data.size() >= sizeof(Core::Mat3)) {
                Core::Mat3 value;
                std::memcpy(&value, data.data(), sizeof(Core::Mat3));
                return value;
            }
            return Core::Mat3(1.0f);
        }

        Core::Mat4 RenderParameterValue::GetMat4() const {
            if (type == Type::Mat4 && data.size() >= sizeof(Core::Mat4)) {
                Core::Mat4 value;
                std::memcpy(&value, data.data(), sizeof(Core::Mat4));
                return value;
            }
            return Core::Mat4Identity();
        }

        // ============================================================================
        // RenderParameterCollector implementation
        // ============================================================================

        RenderParameterCollector::RenderParameterCollector() = default;
        RenderParameterCollector::~RenderParameterCollector() = default;

        void RenderParameterCollector::CollectFromComponent(Resources::ModelComponent* component, Resources::Entity* entity) {
            if (!component || !entity) {
                return;
            }

            // Collect per-object parameters (PerObject uniform buffer)
            // Get world matrix from Entity
            const glm::mat4& worldMatrix = entity->GetWorldMatrix();
            
            // Calculate normal matrix (inverse transpose of upper-left 3x3 of world matrix)
            glm::mat3 normalMatrix3x3 = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));
            // Convert to mat4 (normalMatrix is mat4 in shader, but we only use upper-left 3x3)
            glm::mat4 normalMatrix = glm::mat4(normalMatrix3x3);
            
            // Set PerObject uniform buffer parameters
            // Note: Parameter names must match uniform buffer member names in shader
            // Shader has: modelMatrix, normalMatrix in PerObject cbuffer
            SetParameter("modelMatrix", RenderParameterValue(Core::Mat4(worldMatrix)));
            SetParameter("normalMatrix", RenderParameterValue(Core::Mat4(normalMatrix)));
        }

        void RenderParameterCollector::CollectFromRenderConfig(const RenderConfig* config) {
            if (!config) {
                return;
            }

            // Collect global render config parameters
            // This is a placeholder - actual implementation would collect
            // config-specific parameters (e.g., global uniforms, render flags)
        }

        void RenderParameterCollector::CollectFromMaterialResource(Resources::MaterialResource* materialResource) {
            if (!materialResource) {
                return;
            }

            // Collect material-specific parameters
            // This is a placeholder - actual implementation would collect
            // material parameters (textures, material properties)
        }

        void RenderParameterCollector::CollectFromCamera(
            const Core::Mat4& viewMatrix,
            const Core::Mat4& projMatrix,
            const Core::Mat4& viewProjMatrix
        ) {
            CollectMatrices(viewMatrix, projMatrix, viewProjMatrix);
        }

        void RenderParameterCollector::CollectFromScene(Resources::Scene* scene) {
            if (!scene) {
                return;
            }

            // Collect scene-level parameters (lights, environment, etc.)
            CollectLightData(scene);
        }

        void RenderParameterCollector::CollectFromRenderPass(IRenderPass* pass) {
            if (!pass) {
                return;
            }

            // Collect pass-specific parameters (flags, config)
            CollectPassFlags(pass);
        }

        void RenderParameterCollector::SetParameter(const std::string& key, const RenderParameterValue& value) {
            m_Parameters[key] = value;
        }

        void RenderParameterCollector::Clear() {
            m_Parameters.clear();
        }

        void RenderParameterCollector::Merge(const RenderParameterCollector& other) {
            // Merge parameters from other collector
            // Parameters from 'other' override existing ones with the same key
            for (const auto& [key, value] : other.m_Parameters) {
                m_Parameters[key] = value;
            }
        }

        void RenderParameterCollector::CollectMatrices(
            const Core::Mat4& view,
            const Core::Mat4& proj,
            const Core::Mat4& viewProj
        ) {
            // Set standard camera matrices (PerFrame uniform buffer)
            // Parameter names must match shader member names exactly (case-sensitive)
            SetParameter("viewMatrix", RenderParameterValue(view));
            SetParameter("projectionMatrix", RenderParameterValue(proj));
            SetParameter("viewProjectionMatrix", RenderParameterValue(viewProj));
        }

        void RenderParameterCollector::CollectLightData(Resources::Scene* scene) {
            // Collect light data from scene
            // This is a placeholder - actual implementation would iterate through
            // lights in the scene and collect their parameters
        }

        void RenderParameterCollector::CollectPassFlags(IRenderPass* pass) {
            // Collect pass-specific flags and config
            // This is a placeholder - actual implementation would collect
            // pass-specific parameters (e.g., shadow pass flags, render target config)
        }

    } // namespace Renderer
} // namespace FirstEngine
