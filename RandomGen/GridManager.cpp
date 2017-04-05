#include "GridManager.h"

#include <iostream>
#include <algorithm>
#include <random>       
#include <chrono>       
#include <array>
#include <set>
#include "Math.h"

GridManager::GridManager()
{
}

void GridManager::GenerateMap(int windowWidth, int windowHeight, unsigned int rows, unsigned int columns)
{
	m_windowHeight = windowHeight;
	m_windowWidth = windowWidth;
	m_rowCount = rows;
	m_columnCount = columns;

	m_tileWidth = (float)(m_windowWidth -  (BORDER_WIDTH * (columns))) / (float)m_columnCount;
	m_tileHeight = (float)(m_windowHeight - (BORDER_WIDTH * (rows))) / (float)m_rowCount;

	m_tiles.reserve(m_rowCount);
	std::vector<Tile> row;
	sf::RectangleShape newTile(sf::Vector2f(m_tileWidth, m_tileHeight));
	newTile.setOutlineThickness(0);

	row.resize(m_columnCount, Tile(newTile, Tile::TileType::Empty, BORDER_WIDTH, BORDER_COLOR));
	Tile::TileType tileType;

	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; ++j)
		{
			row[j].SetPosition(sf::Vector2f((m_tileWidth + BORDER_WIDTH) * j, (m_tileHeight + BORDER_WIDTH) * i));
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
	else
		EllersAlgorithmGenerator();
}

void GridManager::RecursiveBacktrackerGenerator()
{
	m_tiles[0][0].SetType(Tile::TileType::Floor);

	CarvePassage(0, 0);
}

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine generator(seed);

//The reason the order is EWNS is that Eller's algorithm only randomly merges in the East and West directions. 
//Ordering it like this means I can just do rand(2) there
std::array<Tile::PassageDirection, 4> DIRECTIONS			= { Tile::East, Tile::West, Tile::North, Tile::South };
std::array<Tile::PassageDirection, 4> OPPOSITE_DIRECTIONS	= { Tile::West, Tile::East, Tile::South, Tile::North };
std::array<std::pair<int, int>, 4>	  DIRECTION_CHANGES		= { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };

void GridManager::CarvePassage(int startI, int startJ)
{
	std::array<int, 4> directionIndices = { 0, 1, 2, 3 };
	shuffle(directionIndices.begin(), directionIndices.end(), generator);
	int nextI, nextJ, index;
	Tile::PassageDirection opposite;
	for (int i = 0; i < 4; ++i)
	{
		index = directionIndices[i];

		nextI = startI + DIRECTION_CHANGES[index].first;
		nextJ = startJ + DIRECTION_CHANGES[index].second;

		if (nextI >= 0 && nextI < m_rowCount && nextJ >= 0 && nextJ < m_columnCount && m_tiles[nextI][nextJ].GetType() == Tile::TileType::Empty)
		{
			m_tiles[nextI][nextJ].AddDirection(OPPOSITE_DIRECTIONS[index]);
			m_tiles[startI][startJ].AddDirection(DIRECTIONS[index]);

			m_tiles[nextI][nextJ].SetType(Tile::TileType::Floor);
			CarvePassage(nextI, nextJ);
		}

	}
}

void GridManager::EllersAlgorithmGenerator()
{
	//Keeps track of the rows sets
	std::vector<std::pair<std::pair<int, int>, int>> rowSets;

	rowSets.resize(m_columnCount, std::make_pair(std::make_pair(-1, -1), -1));
	int lastSet = 0;
	//Keeps track if the set has vertical passage or not
	std::set<int> verticalSet;

	int nextI, nextJ, directionIndex, setIndex;
	bool makeVerticalCut;
	std::pair<int, int> current, next;
	std::uniform_int_distribution<int> distributionHorizontal(0, 2);
	std::uniform_int_distribution<int> distributionVertical(0, 1);

	for (int i = 0; i < m_rowCount; ++i)
	{

		/*
			INITALIZE ROW
		*/
		for (int j = 0; j < m_columnCount; ++j)
		{
			m_tiles[i][j].SetType(Tile::TileType::Floor);
			rowSets[j].first = std::make_pair(i, j);

			if (rowSets[j].second != -1)
				continue;

			rowSets[j].second = lastSet;
			lastSet++;
		}

		/*
			MERGE COLUMN
		*/

		//Last row must merge disjoint sets, and since we cant move down, we exit
		if (i == m_rowCount - 1)
		{
			if (rowSets[0].second != rowSets[1].second)
			{
				m_tiles[i][0].AddDirection(Tile::PassageDirection::East);
				m_tiles[i][1].AddDirection(Tile::PassageDirection::West);
			}

			if (rowSets[m_columnCount - 1].second != rowSets[m_columnCount - 2].second)
			{
				m_tiles[i][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
				m_tiles[i][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
			}

			for (int j = 1; j < m_columnCount - 1; ++j)
			{
				next = current = std::make_pair(i, j);
				next.second = j + 1;

				//If the connection already exists, move on
				if (rowSets[j].second == rowSets[j + 1].second)
					continue;

				//Otherwise connect
				m_tiles[current.first][current.second].AddDirection(Tile::PassageDirection::East);
				m_tiles[next.first][next.second].AddDirection(Tile::PassageDirection::West);
			}

			break;
		}

		//I do first and last tile separately to avoid checking boundaries in loop
		directionIndex = distributionHorizontal(generator);
		
		if (DIRECTIONS[directionIndex] == Tile::PassageDirection::East)
		{
			m_tiles[i][0].AddDirection(Tile::PassageDirection::East);
			m_tiles[i][1].AddDirection(Tile::PassageDirection::West);

			current = std::make_pair(i, 0);
			next = std::make_pair(i, 1);
			
			setIndex = std::min(rowSets[0].second, rowSets[1].second);
			rowSets[0].second = setIndex;
			rowSets[1].second = setIndex;
		}

		directionIndex = distributionHorizontal(generator);
		if (DIRECTIONS[directionIndex] == Tile::PassageDirection::West)
		{
			m_tiles[i][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
			m_tiles[i][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
																					  
			current = std::make_pair(i, m_columnCount - 1);
			next = std::make_pair(i, m_columnCount - 2);

			setIndex = std::min(rowSets[m_columnCount - 1].second, rowSets[m_columnCount - 2].second);
			rowSets[m_columnCount - 1].second = setIndex;
			rowSets[m_columnCount - 2].second = setIndex;
		}

		//Since we only merge left or right, 2 means don't merge
		for (int j = 1; j < m_columnCount - 1; ++j)
		{
			directionIndex = distributionHorizontal(generator);

			if (directionIndex == 2)
				continue;

			next = current = std::make_pair(i, j);
			next.second = current.second + DIRECTION_CHANGES[directionIndex].second;

			//If the connection already exists, move on
			if (rowSets[j].second == rowSets[next.second].second)
				continue;

			//Otherwise connect
			m_tiles[current.first][current.second].AddDirection(DIRECTIONS[directionIndex]);
			m_tiles[next.first][next.second].AddDirection(OPPOSITE_DIRECTIONS[directionIndex]);
			setIndex = std::min(rowSets[j].second, rowSets[next.second].second);
			rowSets[j].second = setIndex;
			rowSets[next.second].second = setIndex;
		}

		/*
			CREATE VERITCAL PASSAGES
		*/

		verticalSet.clear();
		for (int j = 0; j < m_columnCount; ++j)
		{
			makeVerticalCut = distributionVertical(generator);
			setIndex = rowSets[j].second;
			//If we should cut, or this is the last member of the set and we haven't made a cut, make a cut
			if (makeVerticalCut || (!verticalSet.count(setIndex) && (j == m_columnCount - 1 || rowSets[j + 1].second != setIndex)))
			{
				current = rowSets[j].first;
				m_tiles[current.first][current.second].AddDirection(Tile::PassageDirection::South);
				m_tiles[next.first + 1][current.second].AddDirection(Tile::PassageDirection::North);
			}
			else
				rowSets[j].second = -1;
				
		}
		
	}
}