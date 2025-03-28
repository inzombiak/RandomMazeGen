#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include "Tile.h"
#include "MazeGenPriv.h"

#include <vector>
#include <queue>
#include <map>
#include <thread>
#include <future>

/*
	TODO: Maze generators implemented using policies (Modern C++ Design)
*/

class GridManager
{

public:
	~GridManager() { }
	enum SimulationPhase
	{
		GeneratingRooms,
		GeneratingMaze,
		ConnectingMap,
		RemovingDeadEnds,
		Done,
	};

	void GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns);
	void RandomizeMap();
	void SetMazeAlgorithm(MazeDefs::MazeAlgorithm algo);
	void SetMazeGenerateType(MazeDefs::GenerateType type);
	void Close();

	const Tile* GetTiles(int& rows, int& columns);

private:
	void Terminate();

	//Step 1: Generates rooms
	const std::vector<MazeDefs::IntRect>& GenerateRooms();
	//Step 2: Generates maze
	void GenerateMaze();
	void GenerateMazeWorker();
	void GenerateMazeWorkerByStep();
	//Step 3: Connect them
	void ConnectMap();
	void ConnectMapWorker(const std::vector<MazeDefs::IntRect>& rooms);
	void ConnectMapWorkerByStep(std::vector<MazeDefs::IntRect> rooms);
	//Step 4: Remove dead ends
	void RemoveDeadEnds();
	void RemoveDeadEndsWorker();
	void RemoveDeadEndsWorkerByStep();

	int m_windowHeight;
	int m_windowWidth;
	int m_rowCount;
	int m_columnCount;
	float m_tileWidth;
	float m_tileHeight;
	const int BORDER_WIDTH = 2;
	int m_threadSleepTime = 5;
	int m_seed;
	int m_removeDeadEndsPercentage = 75;

	volatile std::atomic<bool> m_terminated;

	SimulationPhase m_simPhase;
	MazeDefs::MazeAlgorithm m_mazeAlgorithm = MazeDefs::RecursiveBacktracker;
	MazeDefs::MazeAlgorithm m_prevMazeAlgo;

	MazeDefs::GenerateType m_mazeGenerateType = MazeDefs::Full;
	MazeDefs::GenerateType m_prevMazeAlgoType = MazeDefs::Full;

	std::thread m_mazeConnectorThread;
	std::thread m_removeDeadEndsThread;

	std::condition_variable m_connectMapCV;
	std::mutex m_connectMapCVMutex;
	std::atomic<bool> m_connectMap = true;

	std::condition_variable m_removeDeadEndsCV;
	std::mutex m_removeDeadEndsCVMutex;
	std::atomic<bool> m_removeDeadEnds = true;

	TileHolder m_tiles;

	std::vector<MazeDefs::IntRect> m_rooms;
};

#endif
