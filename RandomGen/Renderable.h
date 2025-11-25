#ifndef MESH_H
#define MESH_H

#include "Rendering/RenderDefs.h"
#include "Rendering/Texture_D12.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <string>
#include <memory>
#include <vector>

struct Material
{
	std::string m_name;
	std::string m_filePath;

	std::string m_vertexShaderPath;
	std::string m_fragmentShaderPath;

	bool m_isDirty = false;

	std::shared_ptr<Texture_D12> m_textures;
};

struct Mesh
{
	std::string m_name;
	std::string m_filePath;

	std::shared_ptr<Material> m_material;
	
	bool m_isDirty = false;

	std::vector<VertexInput> m_vertices;
	std::vector<unsigned int> m_indices;


};

struct Renderable
{
	std::string m_name;

	std::shared_ptr<Mesh> m_meshs;

	bool m_isDirty = false;

	glm::vec3 m_position;
	glm::quat m_orientation;
	glm::vec3 m_scale;
};

#endif
