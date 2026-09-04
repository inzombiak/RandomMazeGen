#include "MazeGenDefs.h"
#include "GridManager.h"
#include <Core/Singleton.h>
#include "Tile.h"

typedef Core::Singleton<GridManager> GridManagerSingleton;

extern "C" MAZEGENLIB_API void GenerateMap(unsigned int width, unsigned int height, unsigned int rows, unsigned int columns)
{
	GridManagerSingleton::Instance().GenerateMap(width, height, rows, columns);
}
extern "C" MAZEGENLIB_API void SetMazeGenerationType(MazeDefs::GenerateType type) {
	GridManagerSingleton::Instance().SetMazeGenerateType(type);
}
extern "C" MAZEGENLIB_API void SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm algo) {
	GridManagerSingleton::Instance().SetMazeAlgorithm(algo);
}

extern "C" MAZEGENLIB_API const MazeDefs::TileProperties GetTilePropertiesAtIndices(unsigned int i, unsigned int j) {
	return GridManagerSingleton::Instance().GetTileProperties(i, j);
}

extern "C" MAZEGENLIB_API bool IsMazeDirty() {
	return GridManagerSingleton::Instance().IsDirty();
}

extern "C" MAZEGENLIB_API void ClearMazeDirtyFlag() {
	GridManagerSingleton::Instance().ClearDirtyFlag();
}

extern "C" MAZEGENLIB_API void GetAllTileProperties(std::vector<MazeDefs::TileProperties>& buffer) {
	GridManagerSingleton::Instance().GetAllTileProperties(buffer);
}