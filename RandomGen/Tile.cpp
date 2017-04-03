#include "Tile.h"

Tile::Tile(const sf::RectangleShape& tile, const sf::Color& color, const Tile::TileType& type)
{
	m_tile = tile;
	m_tile.setFillColor(color);
	m_type = type;
}

Tile::TileType Tile::GetType() const
{
	return m_type;
}
void Tile::SetType(const TileType& type)
{
	m_type = type;
}
sf::Vector2f Tile::GetPosition() const
{
	return m_tile.getPosition();
}
void Tile::SetPosition(const sf::Vector2f& newPosition)
{
	m_tile.setPosition(newPosition);
}

void Tile::SetColor(const sf::Color& newColor)
{
	m_tile.setFillColor(newColor);
}

void Tile::Draw(sf::RenderWindow& rw)
{
	rw.draw(m_tile);
}
