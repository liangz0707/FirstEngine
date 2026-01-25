#pragma once

#include "FirstEngine/Renderer/Export.h"
#include <glm/glm.hpp>
#include <cstdint>

namespace FirstEngine {
    namespace Renderer {

        // Camera configuration
        struct FE_RENDERER_API CameraConfig {
            CameraConfig() = default;
            
            // Default camera position: looking at origin from a reasonable distance
            // Position: (0, 2, 5) - slightly above and in front of origin
            // Target: (0, 0, 0) - looking at origin
            // This provides a good default view for objects placed at origin
            glm::vec3 position = glm::vec3(0.0f, 2.0f, 5.0f);
            glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            
            float fov = 60.0f;              // Field of view in degrees (60 is a common default)
            float nearPlane = 0.1f;          // Near clipping plane
            float farPlane = 1000.0f;        // Far clipping plane (increased for better visibility)
            
            // Get view matrix
            glm::mat4 GetViewMatrix() const;
            
            // Get projection matrix (requires aspect ratio)
            glm::mat4 GetProjectionMatrix(float aspectRatio) const;
        };

        // Render feature flags
        struct FE_RENDERER_API RenderFlags {
            bool frustumCulling = true;
            bool occlusionCulling = false;
            bool wireframeMode = false;
            bool enableShadows = false;
            bool enablePostProcess = true;
            bool enableDebugDraw = false;
        };

        // Render pipeline type
        enum class RenderPipelineType {
            Deferred,
            Forward,
            Custom
        };

        // Render pipeline configuration
        struct FE_RENDERER_API PipelineConfig {
            RenderPipelineType type = RenderPipelineType::Deferred;
            
            // Deferred rendering specific settings
            struct {
                bool geometryPassEnabled = true;
                bool lightingPassEnabled = true;
                bool postProcessPassEnabled = true;
            } deferredSettings;
            
            // Forward rendering specific settings
            struct {
                bool enableShadows = false;
            } forwardSettings;
        };

        // Resolution configuration
        struct FE_RENDERER_API ResolutionConfig {
            uint32_t width = 1280;
            uint32_t height = 720;
            
            float GetAspectRatio() const {
                return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
            }
        };

        // Global render configuration
        // This class holds all global rendering settings that can be used by
        // SceneRenderer and FrameGraph during rendering pipeline setup
        class FE_RENDERER_API RenderConfig {
        public:
            RenderConfig();
            ~RenderConfig();

            // Camera configuration
            CameraConfig& GetCamera() { return m_Camera; }
            const CameraConfig& GetCamera() const { return m_Camera; }
            void SetCamera(const CameraConfig& camera) { m_Camera = camera; }

            // Resolution configuration
            ResolutionConfig& GetResolution() { return m_Resolution; }
            const ResolutionConfig& GetResolution() const { return m_Resolution; }
            void SetResolution(uint32_t width, uint32_t height) {
                m_Resolution.width = width;
                m_Resolution.height = height;
            }
            void SetResolution(const ResolutionConfig& resolution) { m_Resolution = resolution; }

            // Render flags
            RenderFlags& GetRenderFlags() { return m_RenderFlags; }
            const RenderFlags& GetRenderFlags() const { return m_RenderFlags; }
            void SetRenderFlags(const RenderFlags& flags) { m_RenderFlags = flags; }

            // Pipeline configuration
            PipelineConfig& GetPipelineConfig() { return m_PipelineConfig; }
            const PipelineConfig& GetPipelineConfig() const { return m_PipelineConfig; }
            void SetPipelineConfig(const PipelineConfig& config) { m_PipelineConfig = config; }
            void SetPipelineType(RenderPipelineType type) { m_PipelineConfig.type = type; }
            RenderPipelineType GetPipelineType() const { return m_PipelineConfig.type; }

            // Convenience methods for common operations
            glm::mat4 GetViewMatrix() const { return m_Camera.GetViewMatrix(); }
            glm::mat4 GetProjectionMatrix() const {
                return m_Camera.GetProjectionMatrix(m_Resolution.GetAspectRatio());
            }
            glm::mat4 GetViewProjectionMatrix() const {
                return GetProjectionMatrix() * GetViewMatrix();
            }

            // Update resolution from window
            void UpdateResolutionFromWindow(uint32_t width, uint32_t height) {
                SetResolution(width, height);
            }

        private:
            CameraConfig m_Camera;
            ResolutionConfig m_Resolution;
            RenderFlags m_RenderFlags;
            PipelineConfig m_PipelineConfig;
        };

    } // namespace Renderer
} // namespace FirstEngine
