#ifndef MAZE_DEFS_H
#define MAZE_DEFS_H

#include <chrono>
#include <random> 
#include <array>
#include <map>

#ifdef MAZEGENERATOR_EXPORTS
#define MAZEGENLIB_API __declspec(dllexport)
#else
#define MAZEGENLIB_API __declspec(dllimport)
#endif

class Tile;
namespace MazeDefs
{
	struct Vector2f {
		Vector2f() {
			x = 0;
			y = 0;
		}
		Vector2f(float _x, float _y) {
			x = _x;
			y = _y;
		}
		float x, y;
	};

	struct Vector2i {
		Vector2i() {
			x = 0;
			y = 0;
		}
		Vector2i(int _x, int _y) {
			x = _x;
			y = _y;
		}
		int x, y;
	};

	struct IntRect {
		int top, left, width, height;
		bool intersects(const IntRect& other) {
			if (left + width < other.left)
				return false;
			if (left > other.left + other.width)
				return false;
			if (top + height < other.top)
				return false;
			if (top > other.top + other.height)
				return false;

			return true;
		}
	};

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

	static std::array<PassageDirection, 4>			DIRECTIONS = { MazeDefs::East, MazeDefs::West, MazeDefs::North, MazeDefs::South };
	static std::array<PassageDirection, 4>			OPPOSITE_DIRECTIONS = { MazeDefs::West, MazeDefs::East, MazeDefs::South, MazeDefs::North };;
	static std::array<std::pair<int, int>, 4>		DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };
 
	extern "C" MAZEGENLIB_API void GenerateMap(unsigned int width, unsigned int height, unsigned int rows, unsigned int columns);
	extern "C" MAZEGENLIB_API void SetMazeGenerationType(MazeDefs::GenerateType type);
	extern "C" MAZEGENLIB_API void SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm algo);
	extern "C" MAZEGENLIB_API const Tile* GetGeneratedTiles(int* rows, int* columns);
}

#endif