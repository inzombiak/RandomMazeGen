#include "Tile.h"

Tile::Tile(const sf::RectangleShape& tile, const Tile::TileType& type, int borderWidth, const sf::Color& borderColor)
{
	m_tile = tile;
	m_tile.setFillColor(m_typeToColor[Empty]);
	m_type = type;
	m_direction = PassageDirection::None;
	m_borderWidth = borderWidth;
	m_borderColor = borderColor;
}
Tile::PassageDirection Tile::GetDirection() const
{
	return m_direction;
}
void Tile::SetDirection(const Tile::PassageDirection& dir)
{
	m_direction = dir;
}
void Tile::AddDirection(const PassageDirection& dir)
{
	m_direction = m_direction | dir;
}
Tile::TileType Tile::GetType() const
{
	return m_type;
}
void Tile::SetType(const TileType& type)
{
	m_type = type;
	m_tile.setFillColor(m_typeToColor[type]);
}
sf::Vector2f Tile::GetPosition() const
{
	return m_tile.getPosition();
}
void Tile::SetPosition(const sf::Vector2f& newPosition)
{
	m_tile.setPosition(newPosition);
}

void Tile::Draw(sf::RenderWindow& rw)
{
	rw.draw(m_tile);

	//Draw "walls"
	sf::Vector2f borderPos;
	sf::RectangleShape border;
	

	border.setSize(sf::Vector2f(m_tile.getSize().x, m_borderWidth));

	borderPos = m_tile.getPosition();
	border.setPosition(borderPos);
	border.setFillColor(m_tile.getFillColor());
	if ((m_direction & PassageDirection::North) != PassageDirection::North)
		border.setFillColor(m_borderColor);

	rw.draw(border);

	borderPos = m_tile.getPosition();
	borderPos.y += m_tile.getSize().y;
	border.setPosition(borderPos);
	border.setFillColor(m_tile.getFillColor());
	if ((m_direction & PassageDirection::South) != PassageDirection::South)
		border.setFillColor(m_borderColor);
		
	rw.draw(border);

	border.setSize(sf::Vector2f(m_borderWidth, m_tile.getSize().y));

	borderPos = m_tile.getPosition();
	borderPos.x += m_tile.getSize().x;
	border.setPosition(borderPos);
	border.setFillColor(m_tile.getFillColor());

	if ((m_direction & PassageDirection::East) != PassageDirection::East)
		border.setFillColor(m_borderColor);

	rw.draw(border);

	borderPos = m_tile.getPosition();
	border.setPosition(borderPos);
	border.setFillColor(m_tile.getFillColor());

	if ((m_direction & PassageDirection::West) != PassageDirection::West)
		border.setFillColor(m_borderColor);

	rw.draw(border);
}

void Tile::Reset()
{
	m_type = TileType::Empty;
	m_direction = PassageDirection::None;
}

Tile::TileTypeToColorMap Tile::m_typeToColor = { { Tile::TileType::Empty, sf::Color::White }, { Tile::TileType::Floor, sf::Color(51, 102, 153) } };