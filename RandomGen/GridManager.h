#ifndef GRID_MANAGER_H
#define GRID_MANAGER_H

#include <vector>
#include <queue>
#include <map>

#include "Tile.h"

class GridManager
{

public:
	GridManager();

	void GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns);
	void RandomizeMap();

	void Draw(sf::RenderWindow& rw);

private:


	//TODO: Should asssign to a pointer not to m_currentShape(which should be a pointer)
	bool GetShapeContainingPoint(const sf::Vector2f& point);

	int m_windowHeight;
	int m_windowWidth;
	int m_rowCount;
	int m_columnCount;
	float m_tileWidth;
	float m_tileHeight;
	const int BORDER_WIDTH = 2;
	const sf::Color BORDER_COLOR = sf::Color::White;

	std::vector<std::vector<Tile>> m_tiles;
	typedef std::map<Tile::TileType, sf::Color> TileTypeToColorMap;
	TileTypeToColorMap m_typeToColor;
};

#endif
