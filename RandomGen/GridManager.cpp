#include "GridManager.h"

#include <iostream>
#include <unordered_map>

#include "Math.h"

GridManager::GridManager()
{
	m_typeToColor[Tile::TileType::Floor] = sf::Color::Green;
	m_typeToColor[Tile::TileType::Wall] = sf::Color::Blue;
}

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth -  (2 * BORDER_WIDTH * columns)) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight - (BORDER_WIDTH * rows)) / (float)m_rowCount;

	m_tiles.reserve(m_rowCount);
	std::vector<Tile> row;
	sf::RectangleShape newTile(sf::Vector2f(m_tileWidth, m_tileHeight));
	newTile.setOutlineThickness(BORDER_WIDTH);
	newTile.setOutlineColor(BORDER_COLOR);

	row.resize(m_columnCount, Tile(newTile, m_typeToColor[Tile::TileType::Floor], Tile::TileType::Floor));
	Tile::TileType tileType;

	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			row[j].SetPosition(sf::Vector2f((m_tileWidth + BORDER_WIDTH) * j + BORDER_WIDTH, (m_tileHeight + BORDER_WIDTH) * i + BORDER_WIDTH));
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
	Tile::TileType tileType = Tile::TileType::Floor;
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			tileType = Tile::TileType::Wall;

			if (std::rand() % 100 <= 50)
				tileType = Tile::TileType::Floor;

			m_tiles[i][j].SetType(tileType);
			m_tiles[i][j].SetColor(m_typeToColor[tileType]);
		}
	}
}