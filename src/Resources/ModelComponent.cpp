#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/Scene.h"
#include "FirstEngine/Resources/MeshResource.h"
#include "FirstEngine/Resources/MaterialResource.h"
#include "FirstEngine/Renderer/RenderBatch.h"
#include "FirstEngine/Renderer/RenderFlags.h"
#include "FirstEngine/Renderer/RenderGeometry.h"
#include "FirstEngine/Renderer/ShadingMaterial.h"
#include "FirstEngine/RHI/Types.h"
#include "FirstEngine/RHI/IImage.h"
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#if __has_include(<filesystem>)
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

namespace FirstEngine {
    namespace Resources {

        ModelComponent::ModelComponent() : Component(ComponentType::Mesh) {}

        ModelComponent::~ModelComponent() {
            // Release model reference
            if (m_Model) {
                m_Model->Release();
            }
        }

        void ModelComponent::SetModel(ModelHandle model) {
            if (m_Model == model) return;

            // Release old model
            if (m_Model) {
                m_Model->Release();
            }

            m_Model = model;
            if (m_Model) {
                m_Model->AddRef();
                // RenderGeometry and ShadingMaterial will be created in OnLoad() in Resource handles
            }
        }

        AABB ModelComponent::GetBounds() const {
            if (!m_Model || m_Model->GetMeshCount() == 0) {
                return AABB();
            }

            // Use first mesh for bounds calculation
            MeshHandle mesh = m_Model->GetMesh(0);
            if (!mesh) {
                return AABB();
            }

            // Calculate bounds from mesh data
            const void* vertexData = mesh->GetVertexData();
            if (!vertexData) {
                return AABB();
            }

            // Assuming Vertex structure with position as first member
            const float* positions = static_cast<const float*>(vertexData);
            uint32_t vertexCount = mesh->GetVertexCount();
            uint32_t stride = mesh->GetVertexStride() / sizeof(float);

            if (vertexCount == 0) {
                return AABB();
            }

            glm::vec3 minBounds(positions[0], positions[1], positions[2]);
            glm::vec3 maxBounds = minBounds;

            for (uint32_t i = 1; i < vertexCount; ++i) {
                const float* pos = positions + i * stride;
                glm::vec3 position(pos[0], pos[1], pos[2]);
                minBounds = glm::min(minBounds, position);
                maxBounds = glm::max(maxBounds, position);
            }

            return AABB(minBounds, maxBounds);
        }

        bool ModelComponent::MatchesRenderFlags(Renderer::RenderObjectFlag renderFlags) const {
            if (!m_Model || m_Model->GetMeshCount() == 0) {
                return false;
            }

            // If RenderObjectFlag::All is set, accept all model components
            if ((renderFlags & Renderer::RenderObjectFlag::All) == Renderer::RenderObjectFlag::All) {
                return true;
            }

            // If no flags are set, reject
            if (renderFlags == Renderer::RenderObjectFlag::None) {
                return false;
            }

            // Get material from model to determine if it's transparent or opaque
            Resources::MaterialHandle materialResource = m_Model->GetMaterial(0);
            if (!materialResource) {
                // No material - assume opaque
                return (renderFlags & Renderer::RenderObjectFlag::Opaque) != Renderer::RenderObjectFlag::None;
            }

            // TODO: Check material properties to determine if it's transparent
            // For now, assume all materials are opaque unless explicitly marked
            // This is a placeholder - actual implementation would check material's blend mode
            
            // Default to opaque objects
            bool isOpaque = (renderFlags & Renderer::RenderObjectFlag::Opaque) != Renderer::RenderObjectFlag::None;
            bool isTransparent = (renderFlags & Renderer::RenderObjectFlag::Transparent) != Renderer::RenderObjectFlag::None;
            
            // For now, assume all scene objects are opaque
            // Shadow flag is typically set for all objects that cast shadows
            bool castsShadow = (renderFlags & Renderer::RenderObjectFlag::Shadow) != Renderer::RenderObjectFlag::None;
            
            // Accept if matches opaque flag (most common case)
            if (isOpaque) {
                return true;
            }
            
            // Accept if matches transparent flag
            if (isTransparent) {
                // TODO: Check if material is actually transparent
                // For now, return false as we assume all are opaque
                return false;
            }
            
            // Accept if matches shadow flag (shadow pass)
            if (castsShadow) {
                return true;
            }

            return false;
        }

        void ModelComponent::OnLoad() {
            // Called when Entity is fully loaded (all components attached, resources ready)
            // Create RenderGeometry and ShadingMaterial in Resource handles (not in Component)
            if (!m_Model || m_Model->GetMeshCount() == 0) {
                return;
            }

            // Create RenderGeometry for each mesh in MeshResource handles
            // All creation logic is now encapsulated in MeshResource::CreateRenderGeometry()
            uint32_t meshCount = m_Model->GetMeshCount();
            for (uint32_t i = 0; i < meshCount; ++i) {
                auto* mesh = m_Model->GetMesh(i);
                if (!mesh) {
                    continue;
                }

                // Cast to MeshResource* to access CreateRenderGeometry()
                auto* meshResource = dynamic_cast<MeshResource*>(mesh);
                if (meshResource) {
                    meshResource->CreateRenderGeometry();
                }
            }

            // Create ShadingMaterial for each material in MaterialResource handles
            // All creation logic is now encapsulated in MaterialResource::CreateShadingMaterial()
            for (uint32_t i = 0; i < m_Model->GetMeshCount(); ++i) {
                auto* material = m_Model->GetMaterial(i);
                if (!material) {
                    continue;
                }

                // Cast to MaterialResource* to access CreateShadingMaterial()
                auto* materialResource = dynamic_cast<MaterialResource*>(material);
                if (!materialResource) {
                    continue; // Not a MaterialResource, skip
                }

                // Create ShadingMaterial (all setup logic is inside CreateShadingMaterial())
                materialResource->CreateShadingMaterial();
            }
        }

        std::unique_ptr<Renderer::RenderItem> ModelComponent::CreateRenderItem(
            const glm::mat4& worldMatrix,
            Renderer::RenderObjectFlag renderFlags
        ) {
            // Check if model exists
            if (!m_Model || m_Model->GetMeshCount() == 0) {
                return nullptr;
            }

            // Use first mesh for now (can be extended to support multiple meshes)
            auto* mesh = m_Model->GetMesh(0);
            auto* material = m_Model->GetMaterial(0);
            
            if (!mesh) {
                return nullptr;
            }

            // Get render data from MeshResource (encapsulated, doesn't expose RenderGeometry)
            auto* meshResource = dynamic_cast<MeshResource*>(mesh);
            if (!meshResource || !meshResource->IsRenderGeometryReady()) {
                return nullptr; // Geometry not ready yet
            }

            MeshResource::RenderData geometryData;
            if (!meshResource->GetRenderData(geometryData)) {
                return nullptr; // Failed to get render data
            }

            // Create render item from geometry and material
            auto item = std::make_unique<Renderer::RenderItem>();
            item->entity = m_Entity;
            item->worldMatrix = worldMatrix;
            item->normalMatrix = glm::transpose(glm::inverse(glm::mat3(worldMatrix)));

            // Set geometry data directly
            item->geometryData.vertexBuffer = geometryData.vertexBuffer;
            item->geometryData.indexBuffer = geometryData.indexBuffer;
            item->geometryData.vertexCount = geometryData.vertexCount;
            item->geometryData.indexCount = geometryData.indexCount;
            item->geometryData.firstIndex = geometryData.firstIndex;
            item->geometryData.firstVertex = geometryData.firstVertex;
            item->geometryData.vertexBufferOffset = 0;
            item->geometryData.indexBufferOffset = 0;

            // Get render data from MaterialResource (encapsulated, doesn't expose ShadingMaterial)
            if (material) {
                auto* materialResource = dynamic_cast<MaterialResource*>(material);
                if (materialResource && materialResource->IsShadingMaterialReady()) {
                    MaterialResource::RenderData materialRenderData;
                    if (materialResource->GetRenderData(materialRenderData)) {
                        // Validate vertex inputs match geometry
                        auto* shadingMaterial = static_cast<Renderer::ShadingMaterial*>(materialRenderData.shadingMaterial);
                        if (shadingMaterial) {
                            // Create a temporary RenderGeometry object for validation
                            // We can validate using the vertex stride from geometry data
                            Renderer::RenderGeometry tempGeometry;
                            if (tempGeometry.InitializeFromMesh(mesh)) {
                                // Validate vertex inputs match geometry
                                if (!shadingMaterial->ValidateVertexInputs(&tempGeometry)) {
                                    // Vertex inputs don't match geometry - skip this item
                                    return nullptr;
                                }
                            }
                            
                            // Store material data directly
                            item->materialData.shadingMaterial = materialRenderData.shadingMaterial;
                            item->materialData.pipeline = materialRenderData.pipeline;
                            item->materialData.descriptorSet = materialRenderData.descriptorSet;
                            item->materialData.materialName = materialRenderData.materialName;
                        } else {
                            item->materialData.materialName = material->GetMetadata().name;
                        }
                    } else {
                        item->materialData.materialName = material->GetMetadata().name;
                    }
                } else {
                    item->materialData.materialName = material->GetMetadata().name;
                }
            } else {
                item->materialData.materialName = "Default";
            }

            // Compute sort key
            uint64_t pipelineID = reinterpret_cast<uint64_t>(item->materialData.pipeline) & 0xFFFF;
            uint64_t materialID = std::hash<std::string>{}(item->materialData.materialName) & 0xFFFF;
            float depth = worldMatrix[3][2];
            uint64_t depthKey = static_cast<uint64_t>((depth + 1000.0f) * 1000.0f) & 0xFFFFFFFF;
            item->sortKey = (pipelineID << 48) | (materialID << 32) | depthKey;

            return item;
        }

        Renderer::ShadingMaterial* ModelComponent::GetShadingMaterial() const {
            if (!m_Model || m_Model->GetMeshCount() == 0) {
                return nullptr;
            }

            // Get material from model
            auto* material = m_Model->GetMaterial(0);
            if (!material) {
                return nullptr;
            }

            auto* materialResource = dynamic_cast<MaterialResource*>(material);
            if (!materialResource || !materialResource->IsShadingMaterialReady()) {
                return nullptr;
            }

            // Get ShadingMaterial from MaterialResource
            MaterialResource::RenderData materialRenderData;
            if (!materialResource->GetRenderData(materialRenderData)) {
                return nullptr;
            }

            return static_cast<Renderer::ShadingMaterial*>(materialRenderData.shadingMaterial);
        }

    } // namespace Resources
} // namespace FirstEngine
