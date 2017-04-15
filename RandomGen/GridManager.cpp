#include "GridManager.h"

#include "MGEllers.h"
#include "MGRecursiveBacktracker.h"

#include "RoomGenerator.h"

#include "MazeConnector.h"

#include "DeadEndRemover.h"

#include "Math.h"

#include <iostream>
#include <chrono>     

using namespace GameDefs;

typedef SingletonHolder<MGEllers, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> MGEllersSingleton;
typedef SingletonHolder<MGRecursiveBacktracker, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> MGRecursiveBacktrackerSingleton;
typedef SingletonHolder<RoomGenerator, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> RoomGeneratorSingleton;
typedef SingletonHolder<MazeConnector, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> MazeConnectorSingleton;
typedef SingletonHolder<DeadEndRemover, CreationPolicies::CreateWithNew, LifetimePolicies::DefaultLifetime> DeadEndRemoverSingleton;

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight) / (float)m_rowCount;

	RoomGeneratorSingleton::Instance().SetRoomHorizontalBounds(sf::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetRoomVerticalBounds(sf::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetPlacementAttemptCount(rows*columns/10);

	m_tiles.reserve(m_rowCount);
	std::vector<Tile> row;
	sf::RectangleShape newTile(sf::Vector2f(m_tileWidth, m_tileHeight));
	newTile.setOutlineThickness(0);

	row.resize(m_columnCount, Tile(newTile, TileType::Empty, BORDER_WIDTH, BORDER_COLOR));

	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			row[j].SetPosition(sf::Vector2f(m_tileWidth * j, m_tileHeight  * i));
		}

		m_tiles.push_back(row);
	}

	RandomizeMap();

	m_prevMazeGen = m_mazeGenerator;
	m_prevMazeGenType = m_mazeGenerateType;
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
	if (m_prevMazeGenType == Step)
	{
		m_terminated = true;
		if (m_prevMazeGen == MazeGenerator::RecursiveBacktracker)
			MGRecursiveBacktrackerSingleton::Instance().TerminateGeneration();
		else
			MGEllersSingleton::Instance().TerminateGeneration();

		MazeConnectorSingleton::Instance().TerminateGeneration();
		DeadEndRemoverSingleton::Instance().TerminateGeneration();
	}

	m_prevMazeGenType = m_mazeGenerateType;
	m_prevMazeGen = m_mazeGenerator;
	SetIDManagerSingleton::Instance().Reset();

	//Reset Map
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles[i][j].Reset();
		}
	}

	{
		std::unique_lock<std::mutex> lock(m_connectMapCVMutex);
		m_connectMap = false;
		m_connectMapCV.notify_all();
	}

	{
		std::unique_lock<std::mutex> lock(m_removeDeadEndsCVMutex);
		m_removeDeadEnds = false;
		m_removeDeadEndsCV.notify_all();
	}


	m_terminated = false;
	m_simPhase = GeneratingRooms;
	m_seed = std::chrono::system_clock::now().time_since_epoch().count();

	auto start = std::chrono::high_resolution_clock::now();
	std::vector<sf::IntRect> rooms = GenerateRooms();
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Room generation elapsed time: " << elapsed.count() << std::endl;
	
	m_simPhase = GeneratingMaze;
	GenerateMaze();
	ConnectMap(rooms);
	RemoveDeadEnds();
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

const std::vector<sf::IntRect>& GridManager::GenerateRooms()
{
	return RoomGeneratorSingleton::Instance().GenerateRoom(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
}

void GridManager::GenerateMaze()
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		m_removeDeadEndsThread = std::thread(&GridManager::GenerateMazeWorkerByStep, this);
		m_removeDeadEndsThread.detach();
	}
	else
		GenerateMazeWorker();
}

void GridManager::GenerateMazeWorker()
{
	auto start = std::chrono::high_resolution_clock::now();
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		MGRecursiveBacktrackerSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	else
		MGEllersSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Maze generation elapsed time: " << elapsed.count() << std::endl;
}
void GridManager::GenerateMazeWorkerByStep()
{

	auto start = std::chrono::high_resolution_clock::now();
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		MGRecursiveBacktrackerSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	else
		MGEllersSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Maze generation elapsed time: " << elapsed.count() << std::endl;

	{
		std::unique_lock<std::mutex> lock(m_connectMapCVMutex);
		m_connectMap = true;
		m_connectMapCV.notify_all();
	}
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
		auto start = std::chrono::high_resolution_clock::now();
		ConnectMapWorker(rooms);
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;
		std::cout << "Maze connection elapsed time: " << elapsed.count() << std::endl;
	}

}
void GridManager::ConnectMapWorker(const std::vector<sf::IntRect>& rooms)
{
	MazeConnectorSingleton::Instance().ConnectMaze(rooms, m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
}

void GridManager::ConnectMapWorkerByStep(std::vector<sf::IntRect> rooms)
{
	if (m_terminated)
		return;
	{
		std::unique_lock<std::mutex> lock(m_connectMapCVMutex);
		auto not_paused = [this](){return m_connectMap == true; };
		m_connectMapCV.wait(lock, not_paused);
	}
	if (m_terminated)
		return;

	m_simPhase = ConnectingMap;
	auto start = std::chrono::high_resolution_clock::now();
	MazeConnectorSingleton::Instance().ConnectMaze(rooms, m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Maze connection elapsed time: " << elapsed.count() << std::endl;
	{
		std::unique_lock<std::mutex> lock(m_removeDeadEndsCVMutex);
		m_removeDeadEnds = true;
		m_removeDeadEndsCV.notify_all();
	}
}

void GridManager::RemoveDeadEnds()
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		m_removeDeadEndsThread = std::thread(&GridManager::RemoveDeadEndsWorkerByStep, this);
		m_removeDeadEndsThread.detach();
	}
	else
	{
		m_simPhase = RemovingDeadEnds;
		auto start = std::chrono::high_resolution_clock::now();
		RemoveDeadEndsWorker();
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;
		std::cout << "Dead end removal elapsed time: " << elapsed.count() << std::endl;
	}
}

void GridManager::RemoveDeadEndsWorker()
{
	DeadEndRemoverSingleton::Instance().RemoveDeadEnds(m_tiles, m_mazeGenerateType, m_removeDeadEndsPercentage, m_seed, m_threadSleepTime);
}

void GridManager::RemoveDeadEndsWorkerByStep()
{
	if (m_terminated)
		return;
	{
		std::unique_lock<std::mutex> lock(m_removeDeadEndsCVMutex);
		auto not_paused = [this](){return m_removeDeadEnds == true; };
		m_removeDeadEndsCV.wait(lock, not_paused);
	}

	if (m_terminated)
		return;

	m_simPhase = RemovingDeadEnds;
	auto start = std::chrono::high_resolution_clock::now();
	DeadEndRemoverSingleton::Instance().RemoveDeadEnds(m_tiles, m_mazeGenerateType, m_removeDeadEndsPercentage, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Dead end removal elapsed time: " << elapsed.count() << std::endl;
}