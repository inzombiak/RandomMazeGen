#include "RoomGenerator.h"

using namespace GameDefs;

void RoomGenerator::GenerateRoom(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.size() < 1)
		return;

	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_generateType = genType;
	}

	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_rowCount = (*m_tiles).size();
	if (m_rowCount < 1)
		return;
	m_columnCount = (*m_tiles)[0].size();
	m_rooms.clear();

	if (genType == Step)
		GenerateByStep();
	else
		GenerateFull();
}

void RoomGenerator::GenerateFull()
{
	int attempts;

	{
		std::unique_lock<std::mutex> lock(m_attemptMutex);
		attempts = m_attemptCount;
	}

	bool validRoom;
	std::uniform_int_distribution<int> roomHorizontal(m_horizontalBounds.x, m_horizontalBounds.y);
	std::uniform_int_distribution<int> roomVertical(m_verticalBounds.x, m_verticalBounds.y);
	std::uniform_int_distribution<int> roomX(0, m_columnCount - 1);
	std::uniform_int_distribution<int> roomY(0, m_rowCount - 1);

	while (attempts > 0)
	{
		attempts--;

		sf::IntRect room;
		room.width = roomHorizontal(m_randomNumGen) + 1;
		room.height = roomVertical(m_randomNumGen) + 1;
		room.left = roomX(m_randomNumGen) - 1;
		room.top = roomY(m_randomNumGen) - 1;

		if (room.left + room.width > m_columnCount)
			room.left -= room.left + room.width - m_columnCount;

		if (room.top + room.height > m_rowCount)
			room.top -= room.top + room.height - m_rowCount;

		validRoom = true;
		for (int i = 0; i < m_rooms.size(); ++i)
		{
			if (!room.intersects(m_rooms[i]))
				continue;

			validRoom = false;
			break;
		}

		if (!validRoom)
			continue;

		PassageDirection dirs;
		for (int i = room.top + 1; i < room.top + room.height; ++i)
		{
			for (int j = room.left + 1; j < room.left + room.width; ++j)
			{
				dirs = PassageDirection::North | PassageDirection::South | PassageDirection::East | PassageDirection::West;

				if (i == room.top + 1)
				{
					dirs = dirs & ~PassageDirection::North;
				}
				else if (i == room.top + room.height)
				{
					dirs = dirs & ~PassageDirection::South;
				}
				if (j == room.left + 1)
				{
					dirs = dirs & ~PassageDirection::West;
				}
				else if (j == room.left + room.width)
				{
					dirs = dirs & ~PassageDirection::East;
				}
				(*m_tiles)[i][j].SetType(TileType::Room);
				(*m_tiles)[i][j].SetDirection(dirs);
			}
		}

		m_rooms.push_back(room);

	}
}

void RoomGenerator::SetPlacementAttemptCount(int count)
{
	std::unique_lock<std::mutex> lock(m_attemptMutex);
	m_attemptCount = count;
}

void RoomGenerator::GenerateByStep()
{
	GenerateFull();
}

void RoomGenerator::SetRoomHorizontalBounds(const sf::Vector2i& newHorizBounds)
{
	std::unique_lock<std::mutex> lock(m_horizontalMutex);
	m_horizontalBounds = newHorizBounds;
}
sf::Vector2i RoomGenerator::GetRoomHorizontalBounds()
{
	std::unique_lock<std::mutex> lock(m_horizontalMutex);
	return m_horizontalBounds;
}
void RoomGenerator::SetRoomVerticalBounds(const sf::Vector2i& newVertBounds)
{
	std::unique_lock<std::mutex> lock(m_verticalMutex);
	m_verticalBounds = newVertBounds;
}
sf::Vector2i RoomGenerator::GetRoomVerticalBounds()
{
	std::unique_lock<std::mutex> lock(m_verticalMutex);
	return m_verticalBounds;
}

std::atomic<bool> RoomGenerator::m_done;
std::condition_variable RoomGenerator::m_doneCV;
std::mutex RoomGenerator::m_doneCVMutex;
std::atomic_flag RoomGenerator::m_generate{ 0 };