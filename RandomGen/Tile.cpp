#include "Tile.h"

using namespace GameDefs;

Tile::Tile(const sf::RectangleShape& tile, const TileType& type, int borderWidth, const sf::Color& borderColor)
{
	m_tile = tile;
	//m_tile.setFillColor();
	m_type = type;
	m_direction = PassageDirection::None;
	m_id = -1;

	m_defaultBorderInfo = std::make_pair(borderWidth, borderColor);

	m_borderInfo[PassageDirection::North] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::South] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::East] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::West] = m_defaultBorderInfo;
}
PassageDirection Tile::GetDirection() const
{
	return m_direction;
}
void Tile::SetDirection(const PassageDirection& dir)
{
	m_direction = dir;
}
void Tile::AddDirection(const PassageDirection& dir)
{
	m_direction = m_direction | dir;
}
void Tile::RemoveDirection(const GameDefs::PassageDirection& dir)
{
	m_direction = m_direction & ~dir;
}
bool Tile::HasDirection(const GameDefs::PassageDirection& dir)
{
	return (m_direction & dir) == dir;
}
std::vector<int> Tile::GetPassageDirectionIndices()
{
	std::vector<int> result;
	if (HasDirection(PassageDirection::East))
		result.push_back(0);
	if (HasDirection(PassageDirection::West))
		result.push_back(1);
	if (HasDirection(PassageDirection::North))
		result.push_back(2);
	if (HasDirection(PassageDirection::South))
		result.push_back(3);

	return result;
}
TileType Tile::GetType() const
{
	return m_type;
}
void Tile::SetType(const TileType& type)
{
	m_type = type;
}

void Tile::SetID(int newID)
{
	//TODO: This might be a bad idea
	SetIDManagerSingleton::Instance().RemoveFromSet(m_id);
	SetIDManagerSingleton::Instance().AddToSet(newID);
	m_id = newID;
}
int Tile::GetID() const
{
	return m_id;
}
void Tile::SetColor(const sf::Color& color)
{
	m_tile.setFillColor(color);
}

sf::Vector2f Tile::GetPosition() const
{
	return m_tile.getPosition();
}
void Tile::SetPosition(const sf::Vector2f& newPosition)
{
	m_tile.setPosition(newPosition);
}

void Tile::SetBorder(const GameDefs::PassageDirection& border, int borderWidth, const sf::Color& color)
{
	m_borderInfo[border] = std::make_pair(borderWidth, color);
}

void Tile::Draw(sf::RenderWindow& rw)
{
	rw.draw(m_tile);

	//Dont draw borders for empty tiles
	if (m_type == Empty)
		return;

	//Draw "walls"
	sf::Vector2f borderPos;
	sf::RectangleShape border;

	for (int i = 0; i < 4; ++i)
	{
		borderPos = m_tile.getPosition();
		
		if (DIRECTIONS[i] == PassageDirection::North)
		{
			border.setSize(sf::Vector2f(m_tile.getSize().x, m_borderInfo[DIRECTIONS[i]].first));
		}
		else if (DIRECTIONS[i] == PassageDirection::South)
		{
			borderPos.y += m_tile.getSize().y - m_borderInfo[DIRECTIONS[i]].first;
			border.setSize(sf::Vector2f(m_tile.getSize().x, m_borderInfo[DIRECTIONS[i]].first));
		}
		else if (DIRECTIONS[i] == PassageDirection::East)
		{
			borderPos.x += m_tile.getSize().x - m_borderInfo[DIRECTIONS[i]].first;
			border.setSize(sf::Vector2f(m_borderInfo[DIRECTIONS[i]].first, m_tile.getSize().y));
		}
		else if (DIRECTIONS[i] == PassageDirection::West)
		{
			border.setSize(sf::Vector2f(m_borderInfo[DIRECTIONS[i]].first, m_tile.getSize().y));
		}
			
		border.setPosition(borderPos);
		border.setFillColor(m_tile.getFillColor());

		if (!HasDirection(DIRECTIONS[i]))
			border.setFillColor(m_borderInfo[DIRECTIONS[i]].second);
		else
			continue;

		rw.draw(border);
	}
}

void Tile::Reset()
{
	m_type = TileType::Empty;
	m_direction = PassageDirection::None;
	m_tile.setFillColor(sf::Color::White);
	m_id = -1;

	m_borderInfo[PassageDirection::North] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::South] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::East] = m_defaultBorderInfo;
	m_borderInfo[PassageDirection::West] = m_defaultBorderInfo;

}