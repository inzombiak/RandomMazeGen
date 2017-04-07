#include "MGEllers.h"
#include <algorithm>
#include <random>       
#include <chrono> 
#include <thread>

void MGEllers::GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.size() < 1)
		return;

	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_generateType = genType;
	}

	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_lastSet = 0;
	m_rowCount = (*m_tiles).size();
	if (m_rowCount < 1)
		return;
	m_columnCount = (*m_tiles)[0].size();
	m_rowSets.resize(m_columnCount, std::make_pair(std::make_pair(-1, -1), -1));

	if (genType == Step)
		GenerateByStep();
	else
		GenerateFull();
}

bool MGEllers::CompareAndMergeSets(const std::pair<int, int>& first, const std::pair<int, int>& second)
{
	/*
		I'm going off the assumption that the smaller the 
		set index, the older the set and the higher the chances that it contains more
		tiles and thus easier to merge into than from
	*/

	int firstSetIndex, secondSetIndex, minSetIndex, maxSetIndex;
	firstSetIndex = m_rowSets[first.second].second;
	secondSetIndex = m_rowSets[second.second].second;
	if (firstSetIndex == secondSetIndex)
		return false;

	minSetIndex = std::min(firstSetIndex, secondSetIndex);
	maxSetIndex = std::max(firstSetIndex, secondSetIndex);

	sToIIterator maxIt = m_setToIndices.find(maxSetIndex);
	sToIIterator minIt = m_setToIndices.find(minSetIndex);

	if (maxIt == m_setToIndices.end() || minIt == m_setToIndices.end())
		assert(0);

	auto maxSetTiles = maxIt->second;
	
	for (int i = 0; i < maxSetTiles.size(); ++i)
	{
		m_rowSets[maxSetTiles[i].second].second = minSetIndex;
	}

	minIt->second.insert(minIt->second.end(), maxSetTiles.begin(), maxSetTiles.end());

	m_setToIndices.erase(maxIt);

	return true;
}

void MGEllers::InitalizeRow(int row)
{
	m_setToIndices.clear();
	for (int j = 0; j < m_columnCount; ++j)
	{
		(*m_tiles)[row][j].SetType(Tile::TileType::Floor);
		m_rowSets[j].first = std::make_pair(row, j);

		if (m_rowSets[j].second == -1)
		{
			m_rowSets[j].second = m_lastSet;
			m_lastSet++;
		}
				
		m_setToIndices[m_rowSets[j].second].push_back(m_rowSets[j].first);

	}
}
void MGEllers::MergeColumns(int row)
{
	std::pair<int, int> current, next;
	if (row == m_rowCount - 1)
	{
		if (m_rowSets[0].second != m_rowSets[1].second)
		{
			(*m_tiles)[row][0].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(Tile::PassageDirection::West);
		}

		if (m_rowSets[m_columnCount - 1].second != m_rowSets[m_columnCount - 2].second)
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
		}

		for (int j = 1; j < m_columnCount - 1; ++j)
		{
			next = current = std::make_pair(row, j);
			next.second = j + 1;

			//If the connection already exists, move on
			if (m_rowSets[j].second == m_rowSets[j + 1].second)
				continue;

			//Otherwise connect
			(*m_tiles)[current.first][current.second].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[next.first][next.second].AddDirection(Tile::PassageDirection::West);
		}

		return;
	}

	int nextI, nextJ, directionIndex;
	std::uniform_int_distribution<int> distributionHorizontal(0, 2);
	directionIndex = distributionHorizontal(m_randomNumGen);

	if (DIRECTIONS[directionIndex] == Tile::PassageDirection::East)
	{
		current = std::make_pair(row, 0);
		next = std::make_pair(row, 1);

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][0].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(Tile::PassageDirection::West);
		}
	}

	directionIndex = distributionHorizontal(m_randomNumGen);
	if (DIRECTIONS[directionIndex] == Tile::PassageDirection::West)
	{
		current = std::make_pair(row, m_columnCount - 1);
		next = std::make_pair(row, m_columnCount - 2);

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
		}
	}

	//Since we only merge left or right, 2 means don't merge
	for (int j = 1; j < m_columnCount - 1; ++j)
	{
		directionIndex = distributionHorizontal(m_randomNumGen);

		if (directionIndex == 2)
			continue;

		next = current = std::make_pair(row, j);
		next.second = current.second + DIRECTION_CHANGES[directionIndex].second;

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[current.first][current.second].AddDirection(DIRECTIONS[directionIndex]);
			(*m_tiles)[next.first][next.second].AddDirection(OPPOSITE_DIRECTIONS[directionIndex]);
		}
	}
}
void MGEllers::MakeVerticalCuts(int row)
{
	bool makeVerticalCut;
	std::uniform_int_distribution<int> distributionVertical(0, 1);
	std::set<int> verticalSet;
	verticalSet.clear();
	int setIndex;
	std::pair<int, int> current;

	for (int j = 0; j < m_columnCount; ++j)
	{
		makeVerticalCut = distributionVertical(m_randomNumGen);
		setIndex = m_rowSets[j].second;
		//If we should cut, or this is the last member of the set and we haven't made a cut, make a cut
		if (makeVerticalCut || (!verticalSet.count(setIndex) && (j == m_columnCount - 1 || m_rowSets[j + 1].second != setIndex)))
		{
			current = m_rowSets[j].first;
			(*m_tiles)[current.first][current.second].AddDirection(Tile::PassageDirection::South);
			(*m_tiles)[current.first + 1][current.second].AddDirection(Tile::PassageDirection::North);
		}
		else
			m_rowSets[j].second = -1;

	}
}
void MGEllers::GenerateFull()
{
	for (int i = 0; i < m_rowCount; ++i)
	{
		InitalizeRow(i);
		MergeColumns(i);

		if (i == m_rowCount - 1)
			break;

		MakeVerticalCuts(i);
	}
}

void MGEllers::InitalizeRowByStep(int row)
{
	m_setToIndices.clear();
	for (int j = 0; j < m_columnCount; ++j)
	{
		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

		(*m_tiles)[row][j].SetType(Tile::TileType::Floor);
		m_rowSets[j].first = std::make_pair(row, j);

		if (m_rowSets[j].second == -1)
		{
			m_rowSets[j].second = m_lastSet;
			m_lastSet++;
		}

		m_setToIndices[m_rowSets[j].second].push_back(m_rowSets[j].first);

	}
}
void MGEllers::MergeColumnsByStep(int row)
{
	std::pair<int, int> current, next;
	if (row == m_rowCount - 1)
	{
		if (m_rowSets[0].second != m_rowSets[1].second)
		{
			(*m_tiles)[row][0].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(Tile::PassageDirection::West);
		}

		if (m_rowSets[m_columnCount - 1].second != m_rowSets[m_columnCount - 2].second)
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
		}

		for (int j = 1; j < m_columnCount - 1; ++j)
		{
			next = current = std::make_pair(row, j);
			next.second = j + 1;

			//If the connection already exists, move on
			if (m_rowSets[j].second == m_rowSets[j + 1].second)
				continue;

			//Otherwise connect
			(*m_tiles)[current.first][current.second].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[next.first][next.second].AddDirection(Tile::PassageDirection::West);
		}

		return;
	}

	int nextI, nextJ, directionIndex;
	std::uniform_int_distribution<int> distributionHorizontal(0, 2);
	directionIndex = distributionHorizontal(m_randomNumGen);

	if (DIRECTIONS[directionIndex] == Tile::PassageDirection::East)
	{
		current = std::make_pair(row, 0);
		next = std::make_pair(row, 1);

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][0].AddDirection(Tile::PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(Tile::PassageDirection::West);
		}
	}

	directionIndex = distributionHorizontal(m_randomNumGen);
	if (DIRECTIONS[directionIndex] == Tile::PassageDirection::West)
	{
		current = std::make_pair(row, m_columnCount - 1);
		next = std::make_pair(row, m_columnCount - 2);

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(Tile::PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(Tile::PassageDirection::East);
		}
	}

	//Since we only merge left or right, 2 means don't merge
	for (int j = 1; j < m_columnCount - 1; ++j)
	{
		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		directionIndex = distributionHorizontal(m_randomNumGen);

		if (directionIndex == 2)
			continue;

		next = current = std::make_pair(row, j);
		next.second = current.second + DIRECTION_CHANGES[directionIndex].second;

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[current.first][current.second].AddDirection(DIRECTIONS[directionIndex]);
			(*m_tiles)[next.first][next.second].AddDirection(OPPOSITE_DIRECTIONS[directionIndex]);
		}
	}
}
void MGEllers::MakeVerticalCutsByStep(int row)
{
	bool makeVerticalCut;
	std::uniform_int_distribution<int> distributionVertical(0, 1);
	std::set<int> verticalSet;
	verticalSet.clear();
	int setIndex;
	std::pair<int, int> current;

	for (int j = 0; j < m_columnCount; ++j)
	{
		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		makeVerticalCut = distributionVertical(m_randomNumGen);
		setIndex = m_rowSets[j].second;
		//If we should cut, or this is the last member of the set and we haven't made a cut, make a cut
		if (makeVerticalCut || (!verticalSet.count(setIndex) && (j == m_columnCount - 1 || m_rowSets[j + 1].second != setIndex)))
		{
			current = m_rowSets[j].first;
			(*m_tiles)[current.first][current.second].AddDirection(Tile::PassageDirection::South);
			(*m_tiles)[current.first + 1][current.second].AddDirection(Tile::PassageDirection::North);
		}
		else
			m_rowSets[j].second = -1;

	}
}

void MGEllers::GenerateByStep()
{
	m_generate.test_and_set();
	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_done = false;
		m_doneCV.notify_all();
	}

	for (int i = 0; i < m_rowCount; ++i)
	{
		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			break;
		}
		
		InitalizeRowByStep(i);
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));

		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			break;
		}

		MergeColumnsByStep(i);
		if (i == m_rowCount - 1)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));

		MakeVerticalCutsByStep(i);
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));
	}
	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_done = true;
		m_doneCV.notify_all();
	}

}