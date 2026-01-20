#include "FirstEngine/Renderer/ShadingState.h"
#include "FirstEngine/RHI/IDevice.h"
#include "FirstEngine/RHI/IPipeline.h"
#include "FirstEngine/RHI/IRenderPass.h"
#include "FirstEngine/RHI/Types.h"

namespace FirstEngine {
    namespace Renderer {

        ShadingState::ShadingState() = default;

        ShadingState::~ShadingState() {
            // Pipeline is managed by unique_ptr, will be destroyed automatically
            m_Pipeline.reset();
        }

        bool ShadingState::CreatePipeline(
            RHI::IDevice* device, 
            RHI::IRenderPass* renderPass,
            const std::vector<RHI::VertexInputBinding>& vertexBindings,
            const std::vector<RHI::VertexInputAttribute>& vertexAttributes,
            const std::vector<RHI::DescriptorSetLayoutHandle>& descriptorSetLayouts
        ) {
            if (!device || !renderPass) {
                return false;
            }

            // Build GraphicsPipelineDescription from PipelineState and shader modules
            RHI::GraphicsPipelineDescription pipelineDesc;
            pipelineDesc.renderPass = renderPass;
            pipelineDesc.shaderModules = shaderModules;
            
            // Set vertex input bindings and attributes
            pipelineDesc.vertexBindings = vertexBindings;
            pipelineDesc.vertexAttributes = vertexAttributes;

            // Set descriptor set layouts
            pipelineDesc.descriptorSetLayouts.clear();
            for (RHI::DescriptorSetLayoutHandle layout : descriptorSetLayouts) {
                pipelineDesc.descriptorSetLayouts.push_back(layout);
            }

            // Copy primitive topology
            pipelineDesc.primitiveTopology = pipelineState.primitiveTopology;

            // Copy viewport and scissor
            pipelineDesc.viewport.x = pipelineState.viewport.x;
            pipelineDesc.viewport.y = pipelineState.viewport.y;
            pipelineDesc.viewport.width = pipelineState.viewport.width;
            pipelineDesc.viewport.height = pipelineState.viewport.height;
            pipelineDesc.viewport.minDepth = pipelineState.viewport.minDepth;
            pipelineDesc.viewport.maxDepth = pipelineState.viewport.maxDepth;

            pipelineDesc.scissor.x = pipelineState.scissor.x;
            pipelineDesc.scissor.y = pipelineState.scissor.y;
            pipelineDesc.scissor.width = pipelineState.scissor.width;
            pipelineDesc.scissor.height = pipelineState.scissor.height;

            // Copy rasterization state
            pipelineDesc.rasterizationState = pipelineState.rasterizationState;

            // Copy depth stencil state
            pipelineDesc.depthStencilState = pipelineState.depthStencilState;

            // Copy color blend attachments
            pipelineDesc.colorBlendAttachments = pipelineState.colorBlendAttachments;

            // Create pipeline
            m_Pipeline = device->CreateGraphicsPipeline(pipelineDesc);
            if (!m_Pipeline) {
                return false;
            }

            m_PipelineDirty = false;
            return true;
        }

    } // namespace Renderer
} // namespace FirstEngine
