#include "GridManager.h"

#include "MazeGenPriv.h"

#include "MAEllers.h"
#include "MARecursiveBacktracker.h"

#include "RoomGenerator.h"

#include "MazeConnector.h"

#include "DeadEndRemover.h"

#include <iostream>
#include <chrono>     
#include <print>

using namespace MazeDefs;

typedef Core::Singleton<MAEllers> MAEllersSingleton;
typedef Core::Singleton<MARecursiveBacktracker> MARecursiveBacktrackerSingleton;
typedef Core::Singleton<RoomGenerator> RoomGeneratorSingleton;
typedef Core::Singleton<MazeConnector> MazeConnectorSingleton;
typedef Core::Singleton<DeadEndRemover> DeadEndRemoverSingleton;

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight) / (float)m_rowCount;

	RoomGeneratorSingleton::Instance().SetRoomHorizontalBounds(MazeDefs::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetRoomVerticalBounds(MazeDefs::Vector2i(2, m_rowCount / 5));
	RoomGeneratorSingleton::Instance().SetPlacementAttemptCount(rows*columns/10);
	
	m_tiles.Reset(m_rowCount, m_columnCount);
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles(i, j).SetPosition(MazeDefs::Vector2f(m_tileWidth * j, m_tileHeight  * i));
		}
	}

	RandomizeMap();

	m_prevMazeAlgo = m_mazeAlgorithm;
	m_prevMazeAlgoType = m_mazeGenerateType;

	// Mark as dirty after generation
	m_isDirty = true;
}

void GridManager::Terminate()
{
	m_terminated = true;

	//Remove locks
	{
		std::unique_lock<std::mutex> lock(m_connectMapCVMutex);
		m_connectMap = true;
		m_connectMapCV.notify_all();
	}

	{
		std::unique_lock<std::mutex> lock(m_removeDeadEndsCVMutex);
		m_removeDeadEnds = true;
		m_removeDeadEndsCV.notify_all();
	}


	if (m_prevMazeAlgo == MazeAlgorithm::RecursiveBacktracker)
		MARecursiveBacktrackerSingleton::Instance().TerminateGeneration();
	else
		MAEllersSingleton::Instance().TerminateGeneration();

	MazeConnectorSingleton::Instance().TerminateGeneration();
	DeadEndRemoverSingleton::Instance().TerminateGeneration();
}

const Tile* GridManager::GetTiles(int& rows, int& columns) {
	rows = m_rowCount;
	columns = m_columnCount;
	return m_tiles.GetData();
}


MazeDefs::TileProperties GridManager::GetTileProperties(int row, int col) {
	MazeDefs::TileProperties props;

	if (row >= m_rowCount || row < 0 ||
		col >= m_columnCount || col < 0) {

		std::println("Invalid indices, row {}, col: {}" , row, col);
		return props;
	}

	props.type = m_tiles(row, col).GetType();
	props.directions = m_tiles(row, col).GetPassageDirections();

	return props;
}

void GridManager::GetAllTileProperties(std::vector<MazeDefs::TileProperties>& buffer) {
	unsigned int expectedSize = m_rowCount * m_columnCount;
	if (buffer.empty() || buffer.size() < expectedSize) {
		std::println("Invalid buffer or size. Expected: {}, Got: {}", expectedSize, buffer.size());
		return;
	}

	// Copy all tile properties in row-major order
	for (int i = 0; i < m_rowCount; ++i) {
		for (int j = 0; j < m_columnCount; ++j) {
			int index = i * m_columnCount + j;
			buffer[index].type = m_tiles(i, j).GetType();
			buffer[index].directions = m_tiles(i, j).GetPassageDirections();
		}
	}
}

void GridManager::RandomizeMap()
{
	if (m_prevMazeAlgoType == Step)
	{
		Terminate();
	}

	m_prevMazeAlgoType = m_mazeGenerateType;
	m_prevMazeAlgo = m_mazeAlgorithm;
	SetIDManagerSingleton::Instance().Reset();

	//Reset Map
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles(i, j).Reset();
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
	m_seed = (int)std::chrono::system_clock::now().time_since_epoch().count();

	auto start = std::chrono::high_resolution_clock::now();
	m_rooms = GenerateRooms();
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::println("Room generation elapsed time: {}", elapsed.count());
	
	m_simPhase = GeneratingMaze;
	GenerateMaze();
	ConnectMap();
	RemoveDeadEnds();
	m_simPhase = Done;
}

void GridManager::Close()
{
	MARecursiveBacktrackerSingleton::Instance().TerminateGeneration();
}

void GridManager::SetMazeAlgorithm(MazeAlgorithm algo)
{
	m_mazeAlgorithm = algo;
}
void GridManager::SetMazeGenerateType(GenerateType type)
{
	m_mazeGenerateType = type;
}

const std::vector<MazeDefs::IntRect>& GridManager::GenerateRooms()
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

	// Set dirty callback so algorithm can notify when tiles change
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };

	if (m_mazeAlgorithm == MazeAlgorithm::RecursiveBacktracker) {
		MARecursiveBacktrackerSingleton::Instance().SetDirtyCallback(dirtyCallback);
		MARecursiveBacktrackerSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	} else {
		MAEllersSingleton::Instance().SetDirtyCallback(dirtyCallback);
		MAEllersSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	}

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::println("Maze generation elapsed time: {}", elapsed.count());

	// Mark maze as dirty after generation
	m_isDirty = true;
}
void GridManager::GenerateMazeWorkerByStep()
{

	auto start = std::chrono::high_resolution_clock::now();

	// Set dirty callback so algorithm can notify when tiles change during stepping
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };

	if (m_mazeAlgorithm == MazeAlgorithm::RecursiveBacktracker) {
		MARecursiveBacktrackerSingleton::Instance().SetDirtyCallback(dirtyCallback);
		MARecursiveBacktrackerSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	} else {
		MAEllersSingleton::Instance().SetDirtyCallback(dirtyCallback);
		MAEllersSingleton::Instance().GenerateMaze(m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	}

	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::cout << "Maze generation elapsed time: " << elapsed.count() << std::endl;

	// Mark maze as dirty after each step
	m_isDirty = true;
	if (m_terminated)
		return;
	{
		std::unique_lock<std::mutex> lock(m_connectMapCVMutex);
		m_connectMap = true;
		m_connectMapCV.notify_all();
	}
}

void GridManager::ConnectMap()
{
	if (m_mazeGenerateType == GenerateType::Step)
	{
		m_mazeConnectorThread = std::thread(&GridManager::ConnectMapWorkerByStep, this, m_rooms);
		m_mazeConnectorThread.detach();
	}
	else
	{
		m_simPhase = ConnectingMap;
		auto start = std::chrono::high_resolution_clock::now();
		ConnectMapWorker(m_rooms);
		auto finish = std::chrono::high_resolution_clock::now();
		std::chrono::duration<double> elapsed = finish - start;
		std::println("Maze connection elapsed time: {}", elapsed.count());
	}

}
void GridManager::ConnectMapWorker(const std::vector<MazeDefs::IntRect>& rooms)
{
	// Set dirty callback so connector can notify when tiles change
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };
	MazeConnectorSingleton::Instance().SetDirtyCallback(dirtyCallback);

	MazeConnectorSingleton::Instance().ConnectMaze(rooms, m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	m_isDirty = true;  // Mark dirty after connecting
}

void GridManager::ConnectMapWorkerByStep(std::vector<MazeDefs::IntRect> rooms)
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

	// Set dirty callback so connector can notify when tiles change during stepping
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };
	MazeConnectorSingleton::Instance().SetDirtyCallback(dirtyCallback);

	auto start = std::chrono::high_resolution_clock::now();
	MazeConnectorSingleton::Instance().ConnectMaze(rooms, m_tiles, m_mazeGenerateType, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::println("Maze connection elapsed time: {}", elapsed.count());
	m_isDirty = true;  // Mark dirty after connecting
	if (m_terminated)
		return;
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
		std::println("Dead end removal elapsed time: {}", elapsed.count());
	}
}

void GridManager::RemoveDeadEndsWorker()
{
	// Set dirty callback so dead end remover can notify when tiles change
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };
	DeadEndRemoverSingleton::Instance().SetDirtyCallback(dirtyCallback);

	DeadEndRemoverSingleton::Instance().RemoveDeadEnds(m_tiles, m_mazeGenerateType, m_removeDeadEndsPercentage, m_seed, m_threadSleepTime);
	m_isDirty = true;  // Mark dirty after removing dead ends
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

	// Set dirty callback so dead end remover can notify when tiles change during stepping
	auto dirtyCallback = [this]() { this->SetDirtyFlag(); };
	DeadEndRemoverSingleton::Instance().SetDirtyCallback(dirtyCallback);

	auto start = std::chrono::high_resolution_clock::now();
	DeadEndRemoverSingleton::Instance().RemoveDeadEnds(m_tiles, m_mazeGenerateType, m_removeDeadEndsPercentage, m_seed, m_threadSleepTime);
	auto finish = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed = finish - start;
	std::println("Dead end removal elapsed time: {}", elapsed.count());
	m_isDirty = true;  // Mark dirty after removing dead ends
	m_simPhase = Done;
}