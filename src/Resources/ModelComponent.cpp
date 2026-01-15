#include "FirstEngine/Resources/ModelComponent.h"
#include "FirstEngine/Resources/ResourceProvider.h"
#include "FirstEngine/Resources/ResourceTypes.h"
#include "FirstEngine/Resources/Scene.h"
#include <glm/glm.hpp>

namespace FirstEngine {
    namespace Resources {

        ModelComponent::ModelComponent() : Component(ComponentType::Mesh) {}

        void ModelComponent::SetModel(ModelHandle model) {
            if (m_Model == model) return;

            // Release old model
            if (m_Model) {
                m_Model->Release();
            }

            m_Model = model;
            if (m_Model) {
                m_Model->AddRef();
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


    } // namespace Resources
} // namespace FirstEngine
