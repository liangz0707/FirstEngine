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

        // RenderContext - 统一的渲染上下文，提取 RenderApp 和 EditorAPI 的公共渲染逻辑
        // 这个类封装了 FrameGraph 构建、渲染命令生成和提交的通用流程
        class FE_RENDERER_API RenderContext {
        public:
            // 渲染上下文参数结构（简化：只保留与窗口相关的 swapchain）
            // 其他所有渲染相关参数都由 RenderContext 内部管理
            struct RenderParams {
                RHI::ISwapchain* swapchain = nullptr;  // 必须提供，因为每个 viewport 可能有不同的 swapchain
            };

            RenderContext();
            ~RenderContext();

            RenderContext(const RenderContext&) = delete;
            RenderContext& operator=(const RenderContext&) = delete;
            RenderContext(RenderContext&&) = delete;
            RenderContext& operator=(RenderContext&&) = delete;

            // 开始一帧的渲染准备
            // 包括：释放资源、重建 FrameGraph、构建执行计划、编译 FrameGraph
            // 注意：不包含 Execute FrameGraph，这应该在 OnRender 阶段调用
            // 所有渲染相关参数都从内部获取（device, pipeline, frameGraph, renderConfig 等）
            // 返回：是否成功
            bool BeginFrame();

            // 执行 FrameGraph 生成渲染命令
            // 应该在 BeginFrame 之后、SubmitFrame 之前调用
            // 所有渲染相关参数都从内部获取（frameGraph, scene, renderConfig 等）
            // 返回：是否成功
            bool ExecuteFrameGraph();

            // 处理计划中的资源（可选，可以在 ExecuteFrameGraph 之后调用）
            // 参数：
            //   - device: 设备指针
            //   - maxResourcesPerFrame: 每帧最大处理资源数（0 = 无限制）
            void ProcessResources(RHI::IDevice* device, uint32_t maxResourcesPerFrame = 0);

            // 提交一帧的渲染
            // 包括：等待上一帧、获取图像、记录命令、提交、呈现
            // 参数：
            //   - params: 渲染参数结构（只需要 swapchain，其他都从内部获取）
            // 返回：是否成功
            bool SubmitFrame(const RenderParams& params);

            // 获取内部管理的同步对象（如果使用内部管理）
            RHI::FenceHandle GetInFlightFence() const { return m_InFlightFence; }
            RHI::SemaphoreHandle GetImageAvailableSemaphore() const { return m_ImageAvailableSemaphore; }
            RHI::SemaphoreHandle GetRenderFinishedSemaphore() const { return m_RenderFinishedSemaphore; }

            // 创建内部管理的同步对象（如果外部没有提供）
            bool CreateSyncObjects(RHI::IDevice* device);
            
            // 销毁内部管理的同步对象
            void DestroySyncObjects(RHI::IDevice* device);

            // 获取渲染命令列表（用于调试或自定义处理）
            const RenderCommandList& GetRenderCommands() const { return m_RenderCommands; }
            RenderCommandList& GetRenderCommands() { return m_RenderCommands; }

            // 获取执行计划（用于需要访问执行计划的情况）
            const FrameGraphExecutionPlan& GetExecutionPlan() const { return m_ExecutionPlan; }
            FrameGraphExecutionPlan& GetExecutionPlan() { return m_ExecutionPlan; }

            // ========== 渲染引擎状态管理（用于 EditorAPI） ==========
            
            // 初始化渲染引擎（EditorAPI：使用隐藏 GLFW 窗口）
            // 参数：windowHandle 可选；width, height 初始分辨率
            bool InitializeEngine(void* windowHandle, int width, int height);
            
            // 初始化渲染上下文（RenderApp：使用给定窗口，不创建隐藏窗口）
            // 创建 device、pipeline、frameGraph、同步对象、scene、resource 路径等；不创建 swapchain
            bool InitializeForWindow(void* windowHandle, int width, int height);
            
            // 关闭渲染引擎/上下文
            void ShutdownEngine();
            
            // 检查是否已初始化
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
