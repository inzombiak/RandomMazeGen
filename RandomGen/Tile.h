#ifndef TILE_H
#define TILE_H

#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>

#include "GameDefs.h"

class Tile
{

public:
	Tile(const sf::RectangleShape& tile, const GameDefs::TileType& type, int borderWidth, const sf::Color& borderColor);
	
	GameDefs::PassageDirection GetDirection() const;
	void SetDirection(const GameDefs::PassageDirection& dir);
	void AddDirection(const GameDefs::PassageDirection& dir);
	GameDefs::TileType GetType() const;
	void SetType(const GameDefs::TileType& type);
	sf::Vector2f GetPosition() const;
	void SetPosition(const sf::Vector2f& newPosition);

	void Reset();

	void Draw(sf::RenderWindow& rw);

private:
	typedef std::map<GameDefs::TileType, sf::Color> TileTypeToColorMap;
	static TileTypeToColorMap m_typeToColor;

	sf::RectangleShape m_tile;
	int m_borderWidth;
	sf::Color m_borderColor;

	GameDefs::PassageDirection m_direction;
	GameDefs::TileType m_type;
};


#endif