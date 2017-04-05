#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include <vector>
#include <queue>
#include <map>

#include "Tile.h"

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

	void Draw(sf::RenderWindow& rw);

private:

	//Step 1: Make a maze
	//See http://www.jamisbuck.org/presentations/rubyconf2011/index.html#backtracker-drunk-walk
	void GenerateMaze();
	/*
		I implemented 2 different maze generators
		Based on research Recursive Backtracker is optimal
	*/
	//1. RecursiveBacktracker
	void RecursiveBacktrackerGenerator();
	void CarvePassage(int startI, int startJ);

	//2. Eller's Algorithm
	void EllersAlgorithmGenerator();
	
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
	MazeGenerator m_mazeGenerator = EllersAlgorithm;

	std::vector<std::vector<Tile>> m_tiles;
};

#endif
