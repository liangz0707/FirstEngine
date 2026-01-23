#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/FrameGraph.h"
#include "FirstEngine/Renderer/FrameGraphExecutionPlan.h"
#include "FirstEngine/Renderer/RenderCommandList.h"
#include "FirstEngine/Renderer/CommandRecorder.h"
#include "FirstEngine/Renderer/RenderConfig.h"
#include "FirstEngine/Renderer/IRenderPipeline.h"
#include "FirstEngine/Renderer/DeferredRenderPipeline.h"
#include "FirstEngine/Device/VulkanDevice.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/ISwapchain.h"
#include "FirstEngine/RHI/ICommandBuffer.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/Resources/Scene.h"  // SceneLoader is defined in Scene.h
#include "FirstEngine/Resources/ResourceProvider.h"  // ResourceManager is defined in ResourceProvider.h
#include <memory>
#include <string>
#include <unordered_map>

namespace FirstEngine {
    namespace Renderer {

        // RenderContext - Unified rendering context, extracts common rendering logic from RenderApp and EditorAPI
        // This class encapsulates the common flow of FrameGraph building, render command generation, and submission
        class FE_RENDERER_API RenderContext {
        public:
            // Render context parameter structure (simplified: only keeps window-related swapchain)
            // All other rendering-related parameters are managed internally by RenderContext
            struct RenderParams {
                RHI::ISwapchain* swapchain = nullptr;  // Must be provided, as each viewport may have different swapchain
            };

            RenderContext();
            ~RenderContext();

            RenderContext(const RenderContext&) = delete;
            RenderContext& operator=(const RenderContext&) = delete;
            RenderContext(RenderContext&&) = delete;
            RenderContext& operator=(RenderContext&&) = delete;

            // Begin frame rendering preparation
            // Includes: release resources, rebuild FrameGraph, build execution plan, compile FrameGraph
            // Note: Does not include Execute FrameGraph, this should be called in OnRender phase
            // All rendering-related parameters are obtained internally (device, pipeline, frameGraph, renderConfig, etc.)
            // Returns: whether successful
            bool BeginFrame();

            // Execute FrameGraph to generate render commands
            // Should be called after BeginFrame and before SubmitFrame
            // All rendering-related parameters are obtained internally (frameGraph, scene, renderConfig, etc.)
            // Returns: whether successful
            bool ExecuteFrameGraph();

            // Process scheduled resources (optional, can be called after ExecuteFrameGraph)
            // Parameters:
            //   - device: Device pointer
            //   - maxResourcesPerFrame: Maximum resources to process per frame (0 = unlimited)
            void ProcessResources(RHI::IDevice* device, uint32_t maxResourcesPerFrame = 0);

            // Submit frame rendering
            // Includes: wait for previous frame, acquire image, record commands, submit, present
            // Parameters:
            //   - params: Render parameter structure (only need swapchain, everything else is obtained internally)
            // Returns: whether successful
            bool SubmitFrame(const RenderParams& params);

            // Get internally managed sync objects (if using internal management)
            RHI::FenceHandle GetInFlightFence() const { return m_InFlightFence; }
            RHI::SemaphoreHandle GetImageAvailableSemaphore() const { return m_ImageAvailableSemaphore; }
            RHI::SemaphoreHandle GetRenderFinishedSemaphore() const { return m_RenderFinishedSemaphore; }

            // Create internally managed sync objects (if not provided externally)
            bool CreateSyncObjects(RHI::IDevice* device);
            
            // Destroy internally managed sync objects
            void DestroySyncObjects(RHI::IDevice* device);

            // Get render command list (for debugging or custom processing)
            const RenderCommandList& GetRenderCommands() const { return m_RenderCommands; }
            RenderCommandList& GetRenderCommands() { return m_RenderCommands; }

            // Get execution plan (for cases that need to access execution plan)
            const FrameGraphExecutionPlan& GetExecutionPlan() const { return m_ExecutionPlan; }
            FrameGraphExecutionPlan& GetExecutionPlan() { return m_ExecutionPlan; }

            // ========== Rendering Engine State Management (for EditorAPI) ==========
            
            // Initialize rendering engine (EditorAPI: uses hidden GLFW window)
            // Parameters: windowHandle optional; width, height initial resolution
            bool InitializeEngine(void* windowHandle, int width, int height);
            
            // Initialize rendering context (RenderApp: uses given window, doesn't create hidden window)
            // Creates device, pipeline, frameGraph, sync objects, scene, resource paths, etc.; doesn't create swapchain
            bool InitializeForWindow(void* windowHandle, int width, int height);
            
            // Shutdown rendering engine/context
            void ShutdownEngine();
            
            // Check if initialized
            bool IsEngineInitialized() const { return m_EngineInitialized; }
            
            RHI::IDevice* GetDevice() const { return m_Device; }
            
            IRenderPipeline* GetRenderPipeline() const { return m_RenderPipeline; }
            
            FrameGraph* GetFrameGraph() const { return m_FrameGraph; }
            

            Resources::Scene* GetScene() const { return m_Scene; }
            
            void SetScene(Resources::Scene* scene) { m_Scene = scene; }
            
            RenderConfig& GetRenderConfig() { return m_RenderConfig; }
            const RenderConfig& GetRenderConfig() const { return m_RenderConfig; }
            
            void SetRenderConfig(int width, int height, float renderScale = 1.0f);
            
            bool LoadScene(const std::string& scenePath);
            
            void UnloadScene();

        private:
            bool m_EngineInitialized = false;
            RHI::IDevice* m_Device = nullptr;
            IRenderPipeline* m_RenderPipeline = nullptr;
            FrameGraph* m_FrameGraph = nullptr;
            Resources::Scene* m_Scene = nullptr;
            RenderConfig m_RenderConfig;
            
            RHI::FenceHandle m_InFlightFence = nullptr;
            RHI::SemaphoreHandle m_ImageAvailableSemaphore = nullptr;
            RHI::SemaphoreHandle m_RenderFinishedSemaphore = nullptr;
            
            std::unique_ptr<RHI::ICommandBuffer> m_CommandBuffer;
            CommandRecorder m_CommandRecorder;
            
            FrameGraphExecutionPlan m_ExecutionPlan;
            
            RenderCommandList m_RenderCommands;
            
            uint32_t m_CurrentImageIndex = 0;
            
            void* m_WindowHandle = nullptr;
            
#ifdef _WIN32
            void* m_HiddenGLFWWindow = nullptr;
#endif
        };

    } // namespace Renderer
} // namespace FirstEngine
