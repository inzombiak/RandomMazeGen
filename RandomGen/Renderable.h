#ifndef MESH_H
#define MESH_H

#include "Rendering/RenderDefs.h"
#include "Material.h"

#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

#include <string>
#include <memory>
#include <vector>

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

	unsigned int m_entityData = 0;

	glm::vec3 m_position;
	glm::quat m_orientation;
	glm::vec3 m_scale;
};

#endif
