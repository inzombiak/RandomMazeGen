#ifndef TILE_H
#define TILE_H

#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>

class Tile
{

public:

	enum TileType
	{
		Empty = 0,
		Floor = 1,
	};

	enum PassageDirection
	{
		North = 1,
		East = 2,
		South = 4,
		West = 8,
		None = 16
	};

	Tile(const sf::RectangleShape& tile, const Tile::TileType& type, int borderWidth, const sf::Color& borderColor);
	
	PassageDirection GetDirection() const;
	void SetDirection(const PassageDirection& dir);
	void AddDirection(const PassageDirection& dir);
	TileType GetType() const;
	void SetType(const TileType& type);
	sf::Vector2f GetPosition() const;
	void SetPosition(const sf::Vector2f& newPosition);

	void Reset();

	void Draw(sf::RenderWindow& rw);

private:
	typedef std::map<Tile::TileType, sf::Color> TileTypeToColorMap;
	static TileTypeToColorMap m_typeToColor;

	sf::RectangleShape m_tile;
	int m_borderWidth;
	sf::Color m_borderColor;

	PassageDirection m_direction;
	TileType m_type;
};

inline Tile::PassageDirection operator|(Tile::PassageDirection a, Tile::PassageDirection b)
{
	return static_cast<Tile::PassageDirection>(static_cast<int>(a) | static_cast<int>(b));
}

#endif