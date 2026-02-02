#ifndef MAZE_DEFS_H
#define MAZE_DEFS_H

#include <chrono>
#include <random> 
#include <array>
#include <map>
#include <span>

#ifdef MAZEGENERATOR_EXPORTS
#define MAZEGENLIB_API __declspec(dllexport)
#else
#define MAZEGENLIB_API __declspec(dllimport)
#endif

class Tile;
namespace MazeDefs
{
	enum GenerateType
	{
		Step,
		Full
	};

	static const char* GenerateTypeLabels[2] =
	{
		"Step",
		"Full"
	};

	enum MazeAlgorithm
	{
		RecursiveBacktracker,
		EllersAlgorithm
	};

	static const char* MazeAlgorithmLabels[2] =
	{
		"Recursive Backtracker",
		"EllersAlgorithm"
	};

	enum TileType
	{
		Empty = 0,
		Passage = 1,
		Room = 2,
	};

	enum PassageDirection
	{
		North = 1,
		East = 2,
		South = 4,
		West = 8,
		None = 16
	};

	inline PassageDirection operator|(PassageDirection a, PassageDirection b)
	{
		return static_cast<PassageDirection>(static_cast<int>(a) | static_cast<int>(b));
	}
	inline PassageDirection operator&(PassageDirection a, PassageDirection b)
	{
		return static_cast<PassageDirection>(static_cast<int>(a) & static_cast<int>(b));
	}
	inline PassageDirection operator~(PassageDirection a)
	{
		return static_cast<PassageDirection>(~static_cast<int>(a));
	}

	struct TileProperties {
		TileType type = TileType::Empty;
		PassageDirection directions = PassageDirection::None;
	};

	inline constexpr std::array<PassageDirection, 4>	DIRECTIONS = { MazeDefs::East, MazeDefs::West, MazeDefs::North, MazeDefs::South };
	inline constexpr std::array<PassageDirection, 4>	OPPOSITE_DIRECTIONS = { MazeDefs::West, MazeDefs::East, MazeDefs::South, MazeDefs::North };;
	inline constexpr std::array<std::pair<int, int>, 4> DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };
}

extern "C" {
	MAZEGENLIB_API void GenerateMap(unsigned int width, unsigned int height, unsigned int rows, unsigned int columns);
	MAZEGENLIB_API void SetMazeGenerationType(MazeDefs::GenerateType type);
	MAZEGENLIB_API void SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm algo);
	MAZEGENLIB_API const MazeDefs::TileProperties GetTilePropertiesAtIndices(unsigned int i, unsigned int j);

	// Optimization API: Check if tiles have changed since last query
	MAZEGENLIB_API bool IsMazeDirty();
	MAZEGENLIB_API void ClearMazeDirtyFlag();

	// Batch API: Get all tiles at once (more efficient than individual calls)
	MAZEGENLIB_API void GetAllTileProperties(std::vector<MazeDefs::TileProperties>& buffer);
}


#endif