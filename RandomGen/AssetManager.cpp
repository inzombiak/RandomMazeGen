#include "AssetManager.h"

#include <iostream>

Mesh* AssetManager::LoadMesh(std::string filePath) {
	const struct aiScene* scene = aiImportFile(filePath.c_str(),
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate |
		aiProcess_JoinIdenticalVertices |
		aiProcess_SortByPType);

	// If the import failed, report it
	if (NULL == scene) {
		std::cout << "Failed to load file: " << filePath << "Error: " << (aiGetErrorString()) << std::endl;
		return nullptr;
	}

	Mesh* out = nullptr;
	



	// We're done. Release all resources associated with this import
	aiReleaseImport(scene);
	return out;
}

Material* LoadMaterial(std::string filePath);