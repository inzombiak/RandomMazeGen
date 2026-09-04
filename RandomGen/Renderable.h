#ifndef MESH_H
#define MESH_H
#include <wrl.h>
using namespace Microsoft::WRL;

#include "Rendering/RenderDefs.h"
#include "Material.h"

#include <Core/MathConfig.h>

#include <string>
#include <memory>
#include <vector>
#include <variant>

struct Buffer {
	std::variant<D3D12_CONSTANT_BUFFER_VIEW_DESC, D3D12_INDEX_BUFFER_VIEW, D3D12_VERTEX_BUFFER_VIEW> m_bufferView;
	ComPtr<ID3D12Resource> m_resource;
	SRVAllocation m_srvAlloc;
};

struct Mesh
{
	std::string m_name;
	std::string m_filePath;

	bool m_isDirty = false;
	std::shared_ptr<Material> m_material;

	std::vector<VertexInput> m_vertices;
	std::vector<unsigned int> m_indices;

	Buffer m_vertexBuffer;
	Buffer m_indexBuffer;
};

struct Renderable
{
	std::string m_name;

	std::shared_ptr<Mesh> m_mesh;
	std::shared_ptr<Material> m_material;

	bool m_isDirty = false;

	unsigned int m_entityData = 0;

	glm::vec3 m_position;
	glm::quat m_orientation;
	glm::vec3 m_scale;
};

#endif
