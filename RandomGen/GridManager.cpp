#include "GridManager.h"

#include <iostream>
#include <algorithm>
#include <random>       
#include <chrono>       
#include <array>
#include <unordered_map>

#include "Math.h"

GridManager::GridManager()
{
	m_typeToColor[Tile::TileType::Empty] = sf::Color::White;
	m_typeToColor[Tile::TileType::Floor] = sf::Color(51, 102, 153);
}

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth -  (BORDER_WIDTH * (columns + 1))) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight - (BORDER_WIDTH * (rows + 1))) / (float)m_rowCount;

	m_tiles.reserve(m_rowCount);
	std::vector<Tile> row;
	sf::RectangleShape newTile(sf::Vector2f(m_tileWidth, m_tileHeight));
	newTile.setOutlineThickness(0);

	row.resize(m_columnCount, Tile(newTile, m_typeToColor[Tile::TileType::Empty], Tile::TileType::Empty, BORDER_WIDTH, BORDER_COLOR));
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
	//Reset Map
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles[i][j].Reset();
		}
	}

	//Step 1: Generate a maze
	GenerateMaze();
}

void GridManager::GenerateMaze()
{
	if (m_mazeGenerator == MazeGenerator::RecursiveBacktracker)
		RecursiveBacktrackerGenerator();
}

void GridManager::RecursiveBacktrackerGenerator()
{
	m_tiles[0][0].SetType(Tile::TileType::Floor);
	m_tiles[0][0].SetColor(m_typeToColor[Tile::TileType::Floor]);
	CarvePassage(0, 0);
}

void GridManager::CarvePassage(int startI, int startJ)
{
	std::array<Tile::PassageDirection, 4> directions = { Tile::North, Tile::East, Tile::South, Tile::West };
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	random_shuffle(directions.begin(), directions.end());
	int nextI, nextJ;
	Tile::PassageDirection opposite;
	for (int i = 0; i < 4; ++i)
	{
		nextI = startI;
		nextJ = startJ;

		switch (directions[i])
		{
			case Tile::North:
				nextI = startI - 1;
				opposite = Tile::South;
				break;
			case Tile::East:
				nextJ = startJ + 1;
				opposite = Tile::West;
				break;
			case Tile::South:
				nextI = startI + 1;
				opposite = Tile::North;
				break;
			case Tile::West:
				nextJ = startJ - 1;
				opposite = Tile::East;
				break;
		}

		if (nextI >= 0 && nextI < m_rowCount && nextJ >= 0 && nextJ < m_columnCount && m_tiles[nextI][nextJ].GetType() == Tile::TileType::Empty)
		{
			m_tiles[nextI][nextJ].SetDirection(m_tiles[nextI][nextJ].GetDirection() | opposite);
			m_tiles[startI][startJ].SetDirection(m_tiles[startI][startJ].GetDirection() | directions[i]);

			m_tiles[nextI][nextJ].SetType(Tile::TileType::Floor);
			m_tiles[nextI][nextJ].SetColor(m_typeToColor[Tile::TileType::Floor]);

			CarvePassage(nextI, nextJ);
		}

	}
}