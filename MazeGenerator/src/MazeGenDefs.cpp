#include "MazeGenDefs.h"
#include "GridManager.h"
#include "Singleton.h"
#include "Tile.h"

typedef SingletonHolder<GridManager, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> GridManagerSingleton;

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