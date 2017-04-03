#ifndef TILE_H
#define TILE_H

#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>

class Tile
{

public:

	enum TileType
	{
		Wall = 0,
		Floor = 1,
	};

	Tile(const sf::RectangleShape& tile, const sf::Color& color, const Tile::TileType& type);
	
	TileType GetType() const;
	void SetType(const TileType& type);
	sf::Vector2f GetPosition() const;
	void SetPosition(const sf::Vector2f& newPosition);

	void SetColor(const sf::Color& newColor);

	void Draw(sf::RenderWindow& rw);

private:

	sf::RectangleShape m_tile;

	TileType m_type;
};

#endif