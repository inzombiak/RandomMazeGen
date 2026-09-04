#ifndef RENDER_COMPONENT_H
#define RENDER_COMPONENT_H

#include "ObjectComponent.h"

#include <Core/MathConfig.h>

#include <memory>

struct Mesh;

// What an Entity needs in order to be drawn.
//
// A rewrite rather than a port: Orbitals' RenderComponent held raw OpenGL
// handles (m_vertexBufferObject, m_program, GLuint draw primitives) and issued
// its own draw call. Here it just names a mesh and carries the per-instance
// data the batching renderer wants; Renderer_D12 owns all the GPU state.
class RenderComponent : public ObjectComponent
{
public:
	static const char*  COMPONENT_NAME;
	static const size_t COMPONENT_ID;

	explicit RenderComponent(unsigned int id) { m_id = id; }

	void SetMesh(const std::shared_ptr<Mesh>& mesh) { m_mesh = mesh; }
	const std::shared_ptr<Mesh>& GetMesh() const { return m_mesh; }

	// Selects the texture set in the pixel shader (0 = floor, 1 = wall).
	void SetEntityData(unsigned int data) { m_entityData = data; }
	unsigned int GetEntityData() const { return m_entityData; }

	// Render scale is kept separate from the Entity transform: physics reports a
	// body's transform but not its extents, and the shared box mesh spans -1..+1.
	void SetRenderScale(const glm::vec3& scale) { m_renderScale = scale; }
	const glm::vec3& GetRenderScale() const { return m_renderScale; }

	const char* GetName() const override { return COMPONENT_NAME; }
	size_t GetComponentID() const override { return COMPONENT_ID; }

private:
	std::shared_ptr<Mesh> m_mesh;
	unsigned int          m_entityData = 0;
	glm::vec3             m_renderScale = glm::vec3(1.0f);
};

#endif // RENDER_COMPONENT_H
