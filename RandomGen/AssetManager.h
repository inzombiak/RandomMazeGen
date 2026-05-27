#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <map>

#include "Renderable.h"

class AssetManager {
public:
	Mesh* LoadMesh(std::string filePath);
	Material* LoadMaterial(std::string filePath);

private:
	std::map<std::string, Mesh> m_meshMap;
	std::map<std::string, Material> m_materialMap;
};

#endif