#include "Tile.h"

#include "MazeGenPriv.h"

using namespace MazeDefs;

Tile::Tile()
{
	m_type = TileType::Empty;
	m_direction = PassageDirection::None;
	m_id = -1;
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
void Tile::RemoveDirection(const PassageDirection& dir)
{
	m_direction = m_direction & ~dir;
}
bool Tile::HasDirection(const PassageDirection& dir) const
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

Vector2f Tile::GetPosition() const
{
	return m_position;
}
void Tile::SetPosition(const Vector2f& newPosition)
{
	m_position = newPosition;
}

void Tile::Reset()
{
	m_type = TileType::Empty;
	m_direction = PassageDirection::None;
	m_id = -1;

}