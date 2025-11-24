#ifndef MESH_H
#define MESH_H

#include "Material.h"
#include <string>
#include <memory>

class Mesh
{
public:

private:
	std::string m_name;
	std::string m_filePath;

	Material* m_material;

};

#endif
