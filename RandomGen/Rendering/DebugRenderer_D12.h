#ifndef DEBUG_RENDERER_D12_H
#define DEBUG_RENDERER_D12_H

#include "IDebugDraw.h"

#include <Core/MathConfig.h>

#include <d3d12.h>
#include <wrl.h>

#include <memory>
#include <vector>

class CommandList_D12;
struct PipelineStateObject;

// Host-side implementation of the physics debug-draw seam.
//
// PhysicsWorld emits world-space AABB wireframes, OBB wireframes, contact points
// and contact normals through orb::IDebugDraw. This collects them into one line
// list per frame and draws them with a dedicated line-topology PSO.
//
// The vertex buffer is a single persistently-mapped upload-heap resource that is
// rewritten each frame. That is safe only because Renderer_D12::Render() waits on
// a fence every frame, so no frame's data is ever in flight while the next writes
// it; if that serialisation is removed this needs a per-frame ring.
class DebugRenderer_D12 : public orb::IDebugDraw
{
public:
	struct LineVertex
	{
		glm::vec3 position;
		glm::vec3 color;
	};

	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device2> device, PipelineStateObject* pso);

	// orb::IDebugDraw
	void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) override;
	void DrawPoint(const glm::vec3& p, float size, const glm::vec3& color) override;
	void DrawAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color) override;

	void Clear() { m_vertices.clear(); }
	bool HasGeometry() const { return !m_vertices.empty(); }
	size_t LineCount() const { return m_vertices.size() / 2; }

	// Uploads whatever has accumulated and records the draw. Expects the caller to
	// have already bound the render targets and the scene constant buffer.
	void Record(std::shared_ptr<CommandList_D12> commandList,
	            D3D12_GPU_VIRTUAL_ADDRESS sceneDataAddress);

	PipelineStateObject* GetPSO() const { return m_pso; }

	static const size_t MAX_LINES;

private:
	std::vector<LineVertex> m_vertices;

	PipelineStateObject*                       m_pso = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource>     m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW                   m_vertexBufferView = {};
	uint8_t*                                   m_mapped = nullptr;
	size_t                                     m_capacityVertices = 0;
};

#endif // DEBUG_RENDERER_D12_H
