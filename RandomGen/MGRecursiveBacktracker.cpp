#include "MGRecursiveBacktracker.h"
#include <thread>
#include <mutex>

using namespace GameDefs;

void MGRecursiveBacktracker::GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.size() < 1)
		return;

	{
		std::unique_lock<std::mutex> lock(m_generatingMutex);
		m_generating = true;
	}

	m_generateType = genType;
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;

	m_rowCount = (*m_tiles).size();
	m_columnCount = (*m_tiles)[0].size();

	if (genType == Step)
		GenerateByStep();
	else
		GenerateFull();

	{
		std::unique_lock<std::mutex> lock(m_generatingMutex);
		m_generating = false;
	}
}

void MGRecursiveBacktracker::GenerateFull()
{
	m_done = false;

	int startI = 0 , startJ = 0;

	while (startI < m_rowCount && startJ < m_columnCount)
	{
		if ((*m_tiles)[startI][startJ].GetType() == TileType::Empty)
		{
			(*m_tiles)[startI][startJ].SetType(TileType::Passage);
			CarvePassageFull(startI, startJ);
		}

		
		startI++;
		startJ++;
	}

	m_done = true;
}

void MGRecursiveBacktracker::CarvePassageFull(int startI, int startJ)
{

	std::array<int, 4> directionIndices = { 0, 1, 2, 3 };
	shuffle(directionIndices.begin(), directionIndices.end(), m_randomNumGen);
	int nextI, nextJ, index;

	for (int i = 0; i < 4; ++i)
	{

		index = directionIndices[i];

		nextI = startI + DIRECTION_CHANGES[index].first;
		nextJ = startJ + DIRECTION_CHANGES[index].second;

		if (nextI >= 0 && nextI < m_rowCount &&
			nextJ >= 0 && nextJ < m_columnCount &&
			(*m_tiles)[nextI][nextJ].GetType() == TileType::Empty)
		{

			(*m_tiles)[nextI][nextJ].AddDirection(OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)[startI][startJ].AddDirection(DIRECTIONS[index]);

			(*m_tiles)[nextI][nextJ].SetType(TileType::Passage);

			CarvePassageFull(nextI, nextJ);
		}

	}
}


void MGRecursiveBacktracker::GenerateByStep()
{
	m_generate.test_and_set();
	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_done = false;
		m_doneCV.notify_all();
	}
	
	int startI = 0, startJ = 0;

	while (startI < m_rowCount && startJ < m_columnCount)
	{
		if ((*m_tiles)[startI][startJ].GetType() == TileType::Empty)
		{
			(*m_tiles)[startI][startJ].SetType(TileType::Passage);
			CarvePassageByStep(startI, startJ);
		}


		startI++;
		startJ++;
	}

	{
		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		m_done = true;
		m_doneCV.notify_all();
	}
}

void MGRecursiveBacktracker::CarvePassageByStep(int startI, int startJ)
{
	if (!m_generate.test_and_set())
	{
		m_generate.clear();
		return;
	}

	std::array<int, 4> directionIndices = { 0, 1, 2, 3 };
	shuffle(directionIndices.begin(), directionIndices.end(), m_randomNumGen);
	int nextI, nextJ, index;

	for (int i = 0; i < 4; ++i)
	{
		if (!m_generate.test_and_set())
		{
			m_generate.clear();
			return;
		}

		index = directionIndices[i];

		nextI = startI + DIRECTION_CHANGES[index].first;
		nextJ = startJ + DIRECTION_CHANGES[index].second;

		if (nextI >= 0 && nextI < (*m_tiles).size() && 
			nextJ >= 0 && nextJ < (*m_tiles)[0].size() && 
			(*m_tiles)[nextI][nextJ].GetType() == TileType::Empty)
		{
			if (!m_generate.test_and_set())
			{
				m_generate.clear();
				return;
			}

			(*m_tiles)[nextI][nextJ].AddDirection(OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)[startI][startJ].AddDirection(DIRECTIONS[index]);

			(*m_tiles)[nextI][nextJ].SetType(TileType::Passage);

			std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

			CarvePassageByStep(nextI, nextJ);
		}

	}
}
