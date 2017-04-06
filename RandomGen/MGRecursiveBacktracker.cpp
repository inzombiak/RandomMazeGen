#include "MGRecursiveBacktracker.h"
#include <thread>
#include <mutex>

std::recursive_timed_mutex mutex;

void MGRecursiveBacktracker::GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.size() < 1)
		return;

	m_generateType = genType;
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	(*m_tiles)[0][0].SetType(Tile::TileType::Floor);

	if (genType == Step)
		GenerateByStep();
	else
		GenerateFull();
}

void MGRecursiveBacktracker::GenerateFull()
{
	m_done = false;

	CarvePassageFull(0, 0);

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

		if (nextI >= 0 && nextI < (*m_tiles).size() &&
			nextJ >= 0 && nextJ < (*m_tiles)[0].size() &&
			(*m_tiles)[nextI][nextJ].GetType() == Tile::TileType::Empty)
		{

			(*m_tiles)[nextI][nextJ].AddDirection(OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)[startI][startJ].AddDirection(DIRECTIONS[index]);

			(*m_tiles)[nextI][nextJ].SetType(Tile::TileType::Floor);

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
	
	CarvePassageByStep(0, 0);

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
			(*m_tiles)[nextI][nextJ].GetType() == Tile::TileType::Empty)
		{
			if (!m_generate.test_and_set())
			{
				m_generate.clear();
				return;
			}

			(*m_tiles)[nextI][nextJ].AddDirection(OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)[startI][startJ].AddDirection(DIRECTIONS[index]);

			(*m_tiles)[nextI][nextJ].SetType(Tile::TileType::Floor);

			std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

			CarvePassageByStep(nextI, nextJ);
		}

	}
}
