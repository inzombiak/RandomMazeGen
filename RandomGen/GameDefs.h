#ifndef GAME_DEFS_H
#define GAME_DEFS_H

#include <chrono>
#include <random> 
#include <array>

namespace GameDefs
{

	enum GenerateType
	{
		Step,
		Full
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

	namespace Private
	{
		typedef std::map<int, sf::Color> SetIDToColorMap;
		static SetIDToColorMap SET_ID_TO_COLOR;

		static int LAST_SET_ID = -1;
	}

	inline int GetCurrentSetID()
	{
		return Private::LAST_SET_ID;
	}

	inline int GetNextSetID()
	{
		Private::LAST_SET_ID++;
		return Private::LAST_SET_ID;
	}

	inline sf::Color GetSetColor(int set)
	{
		auto it = Private::SET_ID_TO_COLOR.find(set);
		if (it != Private::SET_ID_TO_COLOR.end())
			return it->second;

		int seed = std::chrono::system_clock::now().time_since_epoch().count();
		std::uniform_int_distribution<int> dist(0, 255);
		std::default_random_engine randomEngine(seed);

		sf::Color color;

		color.a = 255;
		color.r = dist(randomEngine);
		color.g = dist(randomEngine);
		color.b = dist(randomEngine);

		Private::SET_ID_TO_COLOR[set] = color;

		return color;

	}

	static std::array<PassageDirection, 4>			DIRECTIONS = { GameDefs::East, GameDefs::West, GameDefs::North, GameDefs::South };
	static std::array<PassageDirection, 4>			OPPOSITE_DIRECTIONS = { GameDefs::West, GameDefs::East, GameDefs::South, GameDefs::North };;
	static std::array<std::pair<int, int>, 4>		DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };
}


#endif