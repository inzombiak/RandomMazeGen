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
	RoomGeneratorSingleton::Instance().SetPlacementAttemptCount(90);

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

	GenerateRooms();
	GenerateMaze();
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
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		m_mazeGeneratorThread = std::thread(&MGRecursiveBacktracker::GenerateMaze, MGRecursiveBacktrackerSingleton::Instance(), std::ref(m_tiles), m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
	else
		m_mazeGeneratorThread = std::thread(&MGEllers::GenerateMaze, MGEllersSingleton::Instance(), std::ref(m_tiles), m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);

	if (m_mazeGenerateType == GenerateType::Step)
		m_mazeGeneratorThread.detach();
	else
		m_mazeGeneratorThread.join();
}

void GridManager::GenerateRooms()
{
	RoomGeneratorSingleton::Instance().GenerateRoom(m_tiles, m_mazeGenerateType, std::chrono::system_clock::now().time_since_epoch().count(), 25);
}