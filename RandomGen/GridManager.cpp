#include "GridManager.h"

#include "MGEllers.h"
#include "MGRecursiveBacktracker.h"

#include "RoomGenerator.h"

#include "Singleton.h"
#include "Math.h"

#include <iostream>
#include <chrono>     

using namespace GameDefs;

typedef SingletonHolder<MGEllers, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> MGEllersSingleton;
typedef SingletonHolder<MGRecursiveBacktracker, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> MGRecursiveBacktrackerSingleton;
typedef SingletonHolder<RoomGenerator, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> RoomGeneratorSingleton;

GridManager::GridManager()
{

}

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth -  (BORDER_WIDTH * (columns))) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight - (BORDER_WIDTH * (rows))) / (float)m_rowCount;

	RoomGeneratorSingleton::Instance().SetRoomHorizontalBounds(sf::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetRoomVerticalBounds(sf::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetPlacementAttemptCount(rows*columns/2);

	m_tiles.reserve(m_rowCount);
	std::vector<Tile> row;
	sf::RectangleShape newTile(sf::Vector2f(m_tileWidth, m_tileHeight));
	newTile.setOutlineThickness(0);

	row.resize(m_columnCount, Tile(newTile, TileType::Empty, BORDER_WIDTH, BORDER_COLOR));
	TileType tileType;

	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			row[j].SetPosition(sf::Vector2f((m_tileWidth + BORDER_WIDTH) * j, (m_tileHeight + BORDER_WIDTH) * i));
		}

		m_tiles.push_back(row);
	}

	RandomizeMap();
}

void GridManager::Draw(sf::RenderWindow& rw)
{
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles[i][j].Draw(rw);
		}
	}
}

void GridManager::RandomizeMap()
{
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		MGRecursiveBacktrackerSingleton::Instance().TerminateGeneration();
	else
		MGEllersSingleton::Instance().TerminateGeneration();

	//Reset Map
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles[i][j].Reset();
		}
	}
	m_simPhase = GeneratingRooms;

	std::vector<sf::IntRect> rooms = GenerateRooms();
	m_simPhase = GeneratingMaze;
	GenerateMaze();
	ConnectMap(rooms);
	m_simPhase = Done;
}

void GridManager::Close()
{
	MGRecursiveBacktrackerSingleton::Instance().TerminateGeneration();
}

void GridManager::ToggleMazeGenerator()
{
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		m_mazeGenerator = EllersAlgorithm;
	else
		m_mazeGenerator = MazeGenerator::RecursiveBacktracker;
}
void GridManager::ToggleMazeGenerateType()
{
	if (m_mazeGenerateType == GenerateType::Step)
		m_mazeGenerateType = GenerateType::Full;
	else
		m_mazeGenerateType = GenerateType::Step;
}

void GridManager::GenerateMaze()
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
			m_connectMapFuture = std::async(std::launch::async, &MGRecursiveBacktracker::GenerateMaze, MGRecursiveBacktrackerSingleton::Instance(), std::ref(m_tiles), m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
		else
			m_connectMapFuture = std::async(std::launch::async, &MGEllers::GenerateMaze, MGEllersSingleton::Instance(), std::ref(m_tiles), m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
	}
	else
	{
		if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
			MGRecursiveBacktrackerSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
		else
			MGEllersSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
	}
}

const std::vector<sf::IntRect>& GridManager::GenerateRooms()
{
	return RoomGeneratorSingleton::Instance().GenerateRoom(m_tiles, m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
}

template<typename T>
bool future_is_ready(std::future<T>& t){
	return t.wait_for(std::chrono::seconds(30)) == std::future_status::ready;
}

void GridManager::ConnectMap(const std::vector<sf::IntRect>& rooms)
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		m_mazeConnectorThread = std::thread(&GridManager::ConnectMapWorkerByStep, this, rooms);
		m_mazeConnectorThread.detach();
	}
	else
	{
		m_simPhase = ConnectingMap;
		ConnectMapWorker(rooms);
	}

}
void GridManager::ConnectMapWorker(const std::vector<sf::IntRect>& rooms)
{
	m_simPhase = ConnectingMap;

	std::vector<std::pair<std::pair<int, int>, int>> possibleDoors; // Last int is passage direction, see GameDefs::DIRECTIONS
	int maxDoors, currDoors;
	int roomX, roomY, roomW, roomH, row, column, dirIndex;
	std::pair<int, int> tileIndices, nextTileIndices;
	std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> dist(0, 4);
	GetNextSetID();
	std::set<int> connectedSets;
	for (int i = 0; i < rooms.size(); ++i)
	{
		roomX = rooms[i].left;
		roomY = rooms[i].top;
		roomW = rooms[i].width;
		roomH = rooms[i].height;
		maxDoors = 3;
		currDoors = 0;
		possibleDoors.clear();
		for (column = roomX; column < roomX + roomW; ++column)
		{
			if (roomY > 0)
				possibleDoors.push_back(std::make_pair(std::make_pair(roomY, column), 2));
			if (roomY + roomH < m_rowCount)
				possibleDoors.push_back(std::make_pair(std::make_pair(roomY + roomH - 1, column), 3));
		}

		for (row = roomY; row < roomY + roomH; ++row)
		{
			if (roomX > 0)
				possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX), 0));
			if (roomX + roomW < m_rowCount - 1)
				possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX + roomW), 1));;
		}


		//Randomize attempts
		shuffle(possibleDoors.begin(), possibleDoors.end(), engine);
		int j;
		for (j = 0; j < possibleDoors.size() && currDoors < maxDoors; ++j)
		{
			if (true)//(dist(engine) == 0)
			{
				tileIndices = possibleDoors[j].first;

				dirIndex = possibleDoors[j].second;
				nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
				nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;

				if (m_tiles[nextTileIndices.first][nextTileIndices.second].GetID() == m_tiles[tileIndices.first][tileIndices.second].GetID())
					continue;

				m_tiles[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
				m_tiles[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
				m_tiles[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
				m_tiles[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);

				FloodSet(tileIndices, GetCurrentSetID());

				currDoors++;
			}
		}
	}
}

void GridManager::ConnectMapWorkerByStep(std::vector<sf::IntRect> rooms)
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		if (future_is_ready(m_connectMapFuture))
		{
			m_simPhase = ConnectingMap;

		}
	}
	std::vector<std::pair<std::pair<int, int>, int>> possibleDoors; // Last int is passage direction, see GameDefs::DIRECTIONS
	int maxDoors, currDoors;
	int roomX, roomY, roomW, roomH, row, column, dirIndex;
	std::pair<int, int> tileIndices, nextTileIndices;
	std::default_random_engine engine(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> dist(0, 1);
	GetNextSetID();
	std::set<int> connectedSets;
	for (int i = 0; i < rooms.size(); ++i)
	{
		roomX = rooms[i].left;
		roomY = rooms[i].top;
		roomW = rooms[i].width;
		roomH = rooms[i].height;
		maxDoors = 3;
		currDoors = 0;
		possibleDoors.clear();
		for (column = roomX; column < roomX + roomW; ++column)
		{
			if (roomY > 0)
				possibleDoors.push_back(std::make_pair(std::make_pair(roomY, column), 2));
			if (roomY + roomH < m_rowCount)
				possibleDoors.push_back(std::make_pair(std::make_pair(roomY + roomH - 1, column), 3));
		}

		for (row = roomY; row < roomY + roomH; ++row)
		{
			if (roomX > 0)
				possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX), 0));
			if (roomX + roomW < m_rowCount - 1)
				possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX + roomW), 1));;
		}


		//Randomize attempts
		shuffle(possibleDoors.begin(), possibleDoors.end(), engine);
		int j;
		for (j = 0; j < possibleDoors.size() && currDoors < maxDoors; ++j)
		{
			if (true)//(dist(engine) == 0)
			{
				tileIndices = possibleDoors[j].first;

				dirIndex = possibleDoors[j].second;
				nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
				nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;

				if (m_tiles[nextTileIndices.first][nextTileIndices.second].GetID() == m_tiles[tileIndices.first][tileIndices.second].GetID())
					continue;

				m_tiles[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
				m_tiles[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
				m_tiles[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
				m_tiles[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);

				FloodSetByStep(tileIndices, GetCurrentSetID());

				currDoors++;
			}
		}
	}
}

void GridManager::FloodSet(const std::pair<int, int>& indices, int id)
{
	if (indices.first < 0 || indices.first >= m_rowCount
		|| indices.second < 0 || indices.second >= m_columnCount || m_tiles[indices.first][indices.second].GetID() == id)
		return;

	std::pair<int, int> nextIndices;
	m_tiles[indices.first][indices.second].SetID(id);
	m_tiles[indices.first][indices.second].SetColor(GameDefs::GetSetColor(id));

	for (int i = 0; i < 4; ++i)
	{
		if (m_tiles[indices.first][indices.second].HasDirection(DIRECTIONS[i]))
		{
			nextIndices.first = indices.first + DIRECTION_CHANGES[i].first;
			nextIndices.second = indices.second + DIRECTION_CHANGES[i].second;

			FloodSet(nextIndices, id);
		}
	}
}

void GridManager::FloodSetByStep(const std::pair<int, int>& indices, int id)
{
	if (indices.first < 0 || indices.first >= m_rowCount
		|| indices.second < 0 || indices.second >= m_columnCount || m_tiles[indices.first][indices.second].GetID() == id)
		return;

	std::pair<int, int> nextIndices;
	m_tiles[indices.first][indices.second].SetID(id);
	m_tiles[indices.first][indices.second].SetColor(GameDefs::GetSetColor(id));

	for (int i = 0; i < 4; ++i)
	{
		if (m_tiles[indices.first][indices.second].HasDirection(DIRECTIONS[i]))
		{
			nextIndices.first =	indices.first + DIRECTION_CHANGES[i].first;
			nextIndices.second = indices.second + DIRECTION_CHANGES[i].second;
			std::this_thread::sleep_for(std::chrono::milliseconds(25));

			FloodSetByStep(nextIndices, id);
		}
	}
}
