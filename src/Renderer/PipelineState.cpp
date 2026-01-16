#include "FirstEngine/Renderer/PipelineState.h"
#include <functional>

namespace FirstEngine {
    namespace Renderer {

        bool PipelineState::operator==(const PipelineState& other) const {
            // Compare rasterization state
            if (rasterizationState.depthClampEnable != other.rasterizationState.depthClampEnable ||
                rasterizationState.rasterizerDiscardEnable != other.rasterizationState.rasterizerDiscardEnable ||
                rasterizationState.cullMode != other.rasterizationState.cullMode ||
                rasterizationState.frontFaceCounterClockwise != other.rasterizationState.frontFaceCounterClockwise ||
                rasterizationState.depthBiasEnable != other.rasterizationState.depthBiasEnable ||
                rasterizationState.depthBiasConstantFactor != other.rasterizationState.depthBiasConstantFactor ||
                rasterizationState.depthBiasClamp != other.rasterizationState.depthBiasClamp ||
                rasterizationState.depthBiasSlopeFactor != other.rasterizationState.depthBiasSlopeFactor ||
                rasterizationState.lineWidth != other.rasterizationState.lineWidth) {
                return false;
            }

            // Compare depth stencil state
            if (depthStencilState.depthTestEnable != other.depthStencilState.depthTestEnable ||
                depthStencilState.depthWriteEnable != other.depthStencilState.depthWriteEnable ||
                depthStencilState.depthCompareOp != other.depthStencilState.depthCompareOp ||
                depthStencilState.depthBoundsTestEnable != other.depthStencilState.depthBoundsTestEnable ||
                depthStencilState.stencilTestEnable != other.depthStencilState.stencilTestEnable) {
                return false;
            }

            // Compare color blend attachments
            if (colorBlendAttachments.size() != other.colorBlendAttachments.size()) {
                return false;
            }
            for (size_t i = 0; i < colorBlendAttachments.size(); ++i) {
                const auto& a = colorBlendAttachments[i];
                const auto& b = other.colorBlendAttachments[i];
                if (a.blendEnable != b.blendEnable ||
                    a.srcColorBlendFactor != b.srcColorBlendFactor ||
                    a.dstColorBlendFactor != b.dstColorBlendFactor ||
                    a.colorBlendOp != b.colorBlendOp ||
                    a.srcAlphaBlendFactor != b.srcAlphaBlendFactor ||
                    a.dstAlphaBlendFactor != b.dstAlphaBlendFactor ||
                    a.alphaBlendOp != b.alphaBlendOp) {
                    return false;
                }
            }

            // Compare primitive topology
            if (primitiveTopology != other.primitiveTopology) {
                return false;
            }

            return true;
        }

        size_t PipelineState::GetHash() const {
            size_t hash = 0;
            
            // Hash rasterization state
            std::hash<bool> boolHash;
            std::hash<uint32_t> uint32Hash;
            std::hash<float> floatHash;
            
            hash ^= boolHash(rasterizationState.depthClampEnable) << 1;
            hash ^= boolHash(rasterizationState.rasterizerDiscardEnable) << 2;
            hash ^= uint32Hash(static_cast<uint32_t>(rasterizationState.cullMode)) << 3;
            hash ^= boolHash(rasterizationState.frontFaceCounterClockwise) << 4;
            hash ^= boolHash(rasterizationState.depthBiasEnable) << 5;
            hash ^= floatHash(rasterizationState.depthBiasConstantFactor) << 6;
            hash ^= floatHash(rasterizationState.depthBiasClamp) << 7;
            hash ^= floatHash(rasterizationState.depthBiasSlopeFactor) << 8;
            hash ^= floatHash(rasterizationState.lineWidth) << 9;

            // Hash depth stencil state
            hash ^= boolHash(depthStencilState.depthTestEnable) << 10;
            hash ^= boolHash(depthStencilState.depthWriteEnable) << 11;
            hash ^= uint32Hash(static_cast<uint32_t>(depthStencilState.depthCompareOp)) << 12;
            hash ^= boolHash(depthStencilState.depthBoundsTestEnable) << 13;
            hash ^= boolHash(depthStencilState.stencilTestEnable) << 14;

            // Hash color blend attachments
            hash ^= uint32Hash(static_cast<uint32_t>(colorBlendAttachments.size())) << 15;
            for (const auto& attachment : colorBlendAttachments) {
                hash ^= boolHash(attachment.blendEnable) << 16;
                hash ^= uint32Hash(attachment.srcColorBlendFactor) << 17;
                hash ^= uint32Hash(attachment.dstColorBlendFactor) << 18;
                hash ^= uint32Hash(attachment.colorBlendOp) << 19;
                hash ^= uint32Hash(attachment.srcAlphaBlendFactor) << 20;
                hash ^= uint32Hash(attachment.dstAlphaBlendFactor) << 21;
                hash ^= uint32Hash(attachment.alphaBlendOp) << 22;
            }

            // Hash primitive topology
            hash ^= uint32Hash(static_cast<uint32_t>(primitiveTopology)) << 23;

            return hash;
        }

    } // namespace Renderer
} // namespace FirstEngine
