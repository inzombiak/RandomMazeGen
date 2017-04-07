#ifndef GAME_DEFS_H
#define GAME_DEFS_H

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
}

#endif