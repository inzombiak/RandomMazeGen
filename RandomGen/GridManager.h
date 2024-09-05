#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include "Tile.h"
#include "IMazeGenerator.h"

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
	GridManager() {}
	~GridManager() { Terminate(); }
	enum SimulationPhase
	{
		GeneratingRooms,
		GeneratingMaze,
		ConnectingMap,
		RemovingDeadEnds,
		Done,
	};

	enum MazeGenerator
	{
		RecursiveBacktracker,
		EllersAlgorithm
	};

	void GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns);
	void RandomizeMap();
	void ToggleMazeGenerator();
	void ToggleMazeGenerateType();
	void Close();
	void Draw(sf::RenderWindow& rw);

	const std::vector<std::vector<Tile>>& GetTiles() const;

private:
	void Terminate();

	GridManager(const GridManager&obj) {}

	//Step 1: Generates rooms
	const std::vector<sf::IntRect>& GenerateRooms();
	//Step 2: Generates maze
	void GenerateMaze();
	void GenerateMazeWorker();
	void GenerateMazeWorkerByStep();
	//Step 3: Connect them
	void ConnectMap(const std::vector<sf::IntRect>& rooms);
	void ConnectMapWorker(const std::vector<sf::IntRect>& rooms);
	void ConnectMapWorkerByStep(std::vector<sf::IntRect> rooms);
	//Step 4: Remove dead ends
	void RemoveDeadEnds();
	void RemoveDeadEndsWorker();
	void RemoveDeadEndsWorkerByStep();

	//TODO: Should asssign to a pointer not to m_currentShape(which should be a pointer)
	bool GetShapeContainingPoint(const sf::Vector2f& point);

	int m_windowHeight;
	int m_windowWidth;
	int m_rowCount;
	int m_columnCount;
	float m_tileWidth;
	float m_tileHeight;
	const int BORDER_WIDTH = 2;
	int m_threadSleepTime = 5;
	int m_seed;
	int m_removeDeadEndsPercentage = 50;

	const sf::Color BORDER_COLOR = sf::Color::Black;

	volatile std::atomic<bool> m_terminated;

	SimulationPhase m_simPhase;
	MazeGenerator m_mazeGenerator = RecursiveBacktracker;
	MazeGenerator m_prevMazeGen;

	GameDefs::GenerateType m_mazeGenerateType = GameDefs::Full;
	GameDefs::GenerateType m_prevMazeGenType = GameDefs::Full;

	std::thread m_mazeConnectorThread;
	std::thread m_removeDeadEndsThread;

	std::condition_variable m_connectMapCV;
	std::mutex m_connectMapCVMutex;
	std::atomic<bool> m_connectMap = true;

	std::condition_variable m_removeDeadEndsCV;
	std::mutex m_removeDeadEndsCVMutex;
	std::atomic<bool> m_removeDeadEnds = true;

	std::vector<std::vector<Tile>> m_tiles;
};

#endif
