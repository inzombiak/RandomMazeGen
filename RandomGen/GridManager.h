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
	enum SimulationPhase
	{
		GeneratingRooms,
		GeneratingMaze,
		ConnectingMap,
		Done,
	};

	enum MazeGenerator
	{
		RecursiveBacktracker,
		EllersAlgorithm
	};

	GridManager();

	void GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns);
	void RandomizeMap();
	void ToggleMazeGenerator();
	void ToggleMazeGenerateType();
	void Close();
	void Draw(sf::RenderWindow& rw);

private:
	//Step 1: Generates rooms
	const std::vector<sf::IntRect>& GenerateRooms();
	//Step 2: Generates maze
	void GenerateMaze();
	//Step 3: Connect them
	void ConnectMap(const std::vector<sf::IntRect>& rooms);
	void ConnectMapWorker(const std::vector<sf::IntRect>& rooms);
	void FloodSet(const std::pair<int, int>& index, int id);
	void ConnectMapWorkerByStep(std::vector<sf::IntRect> rooms);
	void FloodSetByStep(const std::pair<int, int>& index, int id);

	//TODO: Should asssign to a pointer not to m_currentShape(which should be a pointer)
	bool GetShapeContainingPoint(const sf::Vector2f& point);

	int m_windowHeight;
	int m_windowWidth;
	int m_rowCount;
	int m_columnCount;
	float m_tileWidth;
	float m_tileHeight;
	const int BORDER_WIDTH = 1;
	const sf::Color BORDER_COLOR = sf::Color::Black;

	SimulationPhase m_simPhase;
	MazeGenerator m_mazeGenerator = RecursiveBacktracker;
	GameDefs::GenerateType m_mazeGenerateType = GameDefs::Full;

	std::thread m_mazeGeneratorThread;
	std::future<void> m_connectMapFuture;
	std::thread m_mazeConnectorThread;
	std::vector<std::vector<Tile>> m_tiles;
};

#endif
