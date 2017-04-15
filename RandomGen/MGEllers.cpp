#include "MGEllers.h"
#include <algorithm>
#include <random>       
#include <chrono> 
#include <thread>

using namespace GameDefs;

void MGEllers::GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.size() < 1)
		return;

	{
		std::unique_lock<std::mutex> lock;
		m_generateType = genType;
	}
	m_rowSets.clear();
	m_setToIndices.clear();
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_rowCount = (*m_tiles).size();
	m_columnCount = (*m_tiles)[0].size();
	m_rowSets.resize(m_columnCount, std::make_pair(std::make_pair(-1, -1), -1));
	m_seed = seed;

	if (genType == GameDefs::Step)
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

	sToIIterator firstIt = m_setToIndices.find(firstSetIndex);
	sToIIterator secondIt = m_setToIndices.find(secondSetIndex);
	sToIIterator maxIt, minIt;
	if (firstIt == m_setToIndices.end() || secondIt == m_setToIndices.end())
		return false;
	if (firstIt->second.size() > secondIt->second.size())
	{
		minIt = secondIt;
		minSetIndex = secondSetIndex;
		maxIt = firstIt;
		maxSetIndex = firstSetIndex;
	}
	else
	{
		minIt = firstIt;
		minSetIndex = firstSetIndex;
		maxIt = secondIt;
		maxSetIndex = secondSetIndex;
	}

	if (maxIt == m_setToIndices.end() || minIt == m_setToIndices.end())
		assert(0);

	auto minSetTiles = minIt->second;
	std::pair<int, int> indices;
	for (int i = 0; i < minSetTiles.size(); ++i)
	{
		indices = minSetTiles[i];
		if (indices == INVALID_INDICES)
			continue;
		m_rowSets[minSetTiles[i].second].second = maxSetIndex;
		(*m_tiles)[indices.first][indices.second].SetID(maxSetIndex);
		(*m_tiles)[indices.first][indices.second].SetColor(SetIDManagerSingleton::Instance().GetSetColor(maxSetIndex, m_seed));
	}

	maxIt->second.insert(maxIt->second.end(), minSetTiles.begin(), minSetTiles.end());

	m_setToIndices.erase(minIt);

	return true;
}

void MGEllers::InitalizeRow(int row)
{
	m_setToIndices.clear();
	for (int j = 0; j < m_columnCount; ++j)
	{
		if ((*m_tiles)[row][j].GetType() == GameDefs::Room)
		{
			m_rowSets[j].first = INVALID_INDICES;
			continue;
		}
			
		(*m_tiles)[row][j].SetType(TileType::Passage);
		(*m_tiles)[row][j].SetID(GameDefs::SetIDManagerSingleton::Instance().GetCurrentSetID());
		(*m_tiles)[row][j].SetColor(SetIDManagerSingleton::Instance().GetSetColor(GameDefs::SetIDManagerSingleton::Instance().GetCurrentSetID(), m_seed));
		(*m_tiles)[row][j].SetType(GameDefs::Passage);
		m_rowSets[j].first = std::make_pair(row, j);

		if (m_rowSets[j].second == -1)
		{
//			m_rowSets[j].second = m_lastSet;
			//m_lastSet++;
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
			(*m_tiles)[row][0].AddDirection(PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(PassageDirection::West);
		}

		if (m_rowSets[m_columnCount - 1].second != m_rowSets[m_columnCount - 2].second)
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(PassageDirection::East);
		}

		for (int j = 1; j < m_columnCount - 1; ++j)
		{
			current = m_rowSets[j].first;
			next = m_rowSets[j + 1].first;

			if (current == INVALID_INDICES || next == INVALID_INDICES)
				continue;

			//If the connection already exists, move on
			if (m_rowSets[j].second == m_rowSets[j + 1].second)
				continue;

			//Otherwise connect
			(*m_tiles)[current.first][current.second].AddDirection(PassageDirection::East);
			(*m_tiles)[next.first][next.second].AddDirection(PassageDirection::West);
		}

		return;
	}

	int directionIndex;
	std::uniform_int_distribution<int> distributionHorizontal(0, 2);
	directionIndex = distributionHorizontal(m_randomNumGen);

	if (DIRECTIONS[directionIndex] == PassageDirection::East)
	{
		current = m_rowSets[0].first;
		next = m_rowSets[1].first;

		if (current != INVALID_INDICES && next != INVALID_INDICES &&
			CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][0].AddDirection(PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(PassageDirection::West);
		}
	}

	directionIndex = distributionHorizontal(m_randomNumGen);
	if (DIRECTIONS[directionIndex] == PassageDirection::West)
	{
		current = m_rowSets[m_columnCount - 1].first;
		next = m_rowSets[m_columnCount - 2].first;

		if (current != INVALID_INDICES && next != INVALID_INDICES &&
			CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(PassageDirection::East);
		}
	}

	//Since we only merge left or right, 2 means don't merge
	for (int j = 1; j < m_columnCount - 1; ++j)
	{

		directionIndex = distributionHorizontal(m_randomNumGen);

		if (directionIndex == 2)
			continue;

		current = m_rowSets[j].first;
		next = m_rowSets[j + DIRECTION_CHANGES[directionIndex].second].first;

		if (current == INVALID_INDICES || next == INVALID_INDICES)
			continue;

		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[current.first][current.second].AddDirection(DIRECTIONS[directionIndex]);
			(*m_tiles)[next.first][next.second].AddDirection(OPPOSITE_DIRECTIONS[directionIndex]);
		}
	}
}
void MGEllers::MakeVerticalCuts(int row)
{
	int makeVerticalCut;
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
			if (current == INVALID_INDICES || (*m_tiles)[current.first + 1][current.second].GetType() != TileType::Empty)
				continue;

			(*m_tiles)[current.first][current.second].AddDirection(PassageDirection::South);
			(*m_tiles)[current.first + 1][current.second].AddDirection(PassageDirection::North);
			verticalSet.insert(setIndex);
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
	//m_setToIndices.clear();
	int id;
	for (int j = 0; j < m_columnCount; ++j)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}
		
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		if ((*m_tiles)[row][j].GetType() == GameDefs::Room)
		{
			m_rowSets[j].first = INVALID_INDICES;
			continue;
		}

		if (m_rowSets[j].second == -1)
			id = m_rowSets[j].second = GameDefs::SetIDManagerSingleton::Instance().GetNextSetID();
		else
			id = m_rowSets[j].second;

		(*m_tiles)[row][j].SetType(TileType::Passage);
		(*m_tiles)[row][j].SetID(id);
		(*m_tiles)[row][j].SetColor(SetIDManagerSingleton::Instance().GetSetColor(id, m_seed));
		m_rowSets[j].first = std::make_pair(row, j);


		m_setToIndices[id].push_back(m_rowSets[j].first);

	}
}
void MGEllers::MergeColumnsByStep(int row)
{
	std::pair<int, int> current, next;
	int id;
	if (row == m_rowCount - 1)
	{
		if (m_rowSets[0].second != m_rowSets[1].second)
		{
			(*m_tiles)[row][0].AddDirection(PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(PassageDirection::West);
			CompareAndMergeSets(std::make_pair(row, 0), std::make_pair(row, 1));
		}

		if (m_rowSets[m_columnCount - 1].second != m_rowSets[m_columnCount - 2].second)
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(PassageDirection::East);
			CompareAndMergeSets(std::make_pair(row, m_columnCount - 1), std::make_pair(row, m_columnCount - 2));
		}

		for (int j = 1; j < m_columnCount - 1; ++j)
		{
			current = m_rowSets[j].first;
			next = m_rowSets[j + 1].first;

			if (current == INVALID_INDICES || next == INVALID_INDICES)
				continue;

			//If the connection already exists, move on
			if (!CompareAndMergeSets(current, next))
				continue;

			//Otherwise connect
			(*m_tiles)[current.first][current.second].AddDirection(PassageDirection::East);
			(*m_tiles)[next.first][next.second].AddDirection(PassageDirection::West);
		}
		return;
	}

	int directionIndex;
	std::uniform_int_distribution<int> distributionHorizontal(0, 2);
	directionIndex = distributionHorizontal(m_randomNumGen);

	if (DIRECTIONS[directionIndex] == PassageDirection::East)
	{
		current = m_rowSets[0].first;
		next = m_rowSets[1].first;

		if (current != INVALID_INDICES && next != INVALID_INDICES &&
			CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][0].AddDirection(PassageDirection::East);
			(*m_tiles)[row][1].AddDirection(PassageDirection::West);
		}
	}

	directionIndex = distributionHorizontal(m_randomNumGen);
	if (DIRECTIONS[directionIndex] == PassageDirection::West)
	{
		current = m_rowSets[m_columnCount - 1].first;
		next = m_rowSets[m_columnCount - 2].first;

		if (current != INVALID_INDICES && next != INVALID_INDICES &&
			CompareAndMergeSets(current, next))
		{
			(*m_tiles)[row][m_columnCount - 1].AddDirection(PassageDirection::West);
			(*m_tiles)[row][m_columnCount - 2].AddDirection(PassageDirection::East);
		}
	}

	//Since we only merge left or right, 2 means don't merge
	for (int j = 1; j < m_columnCount - 1; ++j)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		directionIndex = distributionHorizontal(m_randomNumGen);

		if (directionIndex == 2)
			continue;

		current = m_rowSets[j].first;
		next = m_rowSets[j + DIRECTION_CHANGES[directionIndex].second].first;

		if (current == INVALID_INDICES || next == INVALID_INDICES)
			continue;
		if (CompareAndMergeSets(current, next))
		{
			(*m_tiles)[current.first][current.second].AddDirection(DIRECTIONS[directionIndex]);
			(*m_tiles)[next.first][next.second].AddDirection(OPPOSITE_DIRECTIONS[directionIndex]);
		}
	}
}
void MGEllers::MakeVerticalCutsByStep(int row)
{
	int makeVerticalCut;
	std::uniform_int_distribution<int> distributionVertical(0, 1);
	std::set<int> verticalSet;
	verticalSet.clear();
	int setIndex, id;
	std::pair<int, int> current;

	for (int j = 0; j < m_columnCount; ++j)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		makeVerticalCut = distributionVertical(m_randomNumGen);
		setIndex = m_rowSets[j].second;
		//If we should cut, or this is the last member of the set and we haven't made a cut, make a cut
		if (makeVerticalCut || (!verticalSet.count(setIndex) && (j == m_columnCount - 1 || m_rowSets[j + 1].second != setIndex)))
		{
			current = m_rowSets[j].first;

			if (current == INVALID_INDICES || (*m_tiles)[current.first + 1][current.second].GetType() != TileType::Empty)
				continue;

			(*m_tiles)[current.first][current.second].AddDirection(PassageDirection::South);
			(*m_tiles)[current.first + 1][current.second].AddDirection(PassageDirection::North);
		}
		else
			m_rowSets[j].second = -1;

	}
}

void MGEllers::GenerateByStep()
{
	CanGenerate();
	SetDoneState(false);

	for (int i = 0; i < m_rowCount; ++i)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}
		
		InitalizeRowByStep(i);
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));

		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		MergeColumnsByStep(i);
		if (i == m_rowCount - 1)
			break;
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));

		MakeVerticalCutsByStep(i);
		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration * 2));
	}

	SetDoneState(true);

}

const std::pair<int, int> MGEllers::INVALID_INDICES = std::make_pair(-1, -1);