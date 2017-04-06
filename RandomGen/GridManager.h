#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include "Tile.h"
#include "IMazeGenerator.h"

#include <vector>
#include <queue>
#include <map>
#include <thread>

/*
	TODO: Maze generators implemented using policies (Modern C++ Design)
*/

class GridManager
{

public:

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

	void Draw(sf::RenderWindow& rw);

private:

	//Generates maze, use GenerateMazeByStep to visiualize
	void GenerateMaze();

	//TODO: Should asssign to a pointer not to m_currentShape(which should be a pointer)
	bool GetShapeContainingPoint(const sf::Vector2f& point);

	int m_windowHeight;
	int m_windowWidth;
	int m_rowCount;
	int m_columnCount;
	float m_tileWidth;
	float m_tileHeight;
	const int BORDER_WIDTH = 1;
	const sf::Color BORDER_COLOR = sf::Color::Red;

	MazeGenerator m_mazeGenerator = RecursiveBacktracker;
	GenerateType m_mazeGenerateType = Full;

	std::thread m_mazeGeneratorThread;
	std::vector<std::vector<Tile>> m_tiles;
};

#endif
