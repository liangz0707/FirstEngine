#pragma once

#include "FirstEngine/Renderer/Export.h"
#include "FirstEngine/Renderer/PipelineState.h"
#include "FirstEngine/RHI/IShaderModule.h"
#include "FirstEngine/RHI/IPipeline.h"
#include <vector>
#include <memory>

namespace FirstEngine {
    namespace RHI {
        class IDevice;
    }

    namespace Renderer {

        // ShadingState - records PipelineState and device-specific Shader modules
        // This is the combination of fixed-function state and programmable shaders
        class FE_RENDERER_API ShadingState {
        public:
            ShadingState();
            ~ShadingState();

            // Pipeline state (fixed-function)
            PipelineState pipelineState;

            // Shader modules (programmable)
            // These are device-specific shader modules (compiled SPIR-V)
            std::vector<RHI::IShaderModule*> shaderModules;

            // Created pipeline (cached, created from pipelineState + shaderModules)
            // This is created when the ShadingState is finalized
            RHI::IPipeline* GetPipeline() const { return m_Pipeline.get(); }
            void SetPipeline(std::unique_ptr<RHI::IPipeline> pipeline) { m_Pipeline = std::move(pipeline); }

            // Check if pipeline needs to be recreated
            bool IsPipelineDirty() const { return m_PipelineDirty; }
            void MarkPipelineDirty() { m_PipelineDirty = true; }
            void ClearPipelineDirty() { m_PipelineDirty = false; }

            // Create pipeline from this state (called by device)
            // This will create the actual GPU pipeline object
            // vertexBindings: Vertex input bindings (from ShadingMaterial)
            // vertexAttributes: Vertex input attributes (from ShadingMaterial)
            // descriptorSetLayouts: Descriptor set layouts (from ShadingMaterial)
            // Returns true if successful, false otherwise
            bool CreatePipeline(
                RHI::IDevice* device, 
                RHI::IRenderPass* renderPass,
                const std::vector<RHI::VertexInputBinding>& vertexBindings,
                const std::vector<RHI::VertexInputAttribute>& vertexAttributes,
                const std::vector<RHI::DescriptorSetLayoutHandle>& descriptorSetLayouts = {}
            );

        private:
            std::unique_ptr<RHI::IPipeline> m_Pipeline;
            bool m_PipelineDirty = true;
        };

    } // namespace Renderer
} // namespace FirstEngine
