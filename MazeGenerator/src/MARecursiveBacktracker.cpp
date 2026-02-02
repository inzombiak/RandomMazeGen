#include "MARecursiveBacktracker.h"
#include "MazeGenPriv.h"

#include <thread>
#include <mutex>

using namespace MazeDefs;

void MARecursiveBacktracker::GenerateMaze(const TileHolder& tiles, const GenerateType& genType, unsigned seed, int sleepDuration)
{
	if (tiles.GetRowCount() < 1)
		return;

	m_generateType = genType;
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_seed = seed;
	m_rowCount = tiles.GetRowCount();
	m_columnCount = tiles.GetColumnCount();

	if (genType == Step)
		GenerateByStep();
	else
		GenerateFull();

}

void MARecursiveBacktracker::GenerateFull()
{
	int startI = 0 , startJ = 0;
	int id;
	while (startI < m_rowCount)
	{
		startJ = 0;

		while (startJ < m_columnCount)
		{
			if ((*m_tiles)(startI, startJ).GetType() == TileType::Empty)
			{
				id = SetIDManagerSingleton::Instance().GetNextSetID();
				(*m_tiles)(startI, startJ).SetType(TileType::Passage);
				(*m_tiles)(startI, startJ).SetID(id);
				CarvePassageFull(startI, startJ);
			}

			startJ++;
		}

		startI++;
	}
}

void MARecursiveBacktracker::CarvePassageFull(int startI, int startJ)
{

	std::array<int, 4> directionIndices = { 0, 1, 2, 3 };
	std::ranges::shuffle(directionIndices, m_randomNumGen);
	int nextI, nextJ, index, id;

	for (int i = 0; i < 4; ++i)
	{

		index = directionIndices[i];
		auto [deltaI, deltaJ] = DIRECTION_CHANGES[index];

		nextI = startI + deltaI;
		nextJ = startJ + deltaJ;

		if (nextI >= 0 && nextI < m_rowCount &&
			nextJ >= 0 && nextJ < m_columnCount &&
			(*m_tiles)(nextI, nextJ).GetType() == TileType::Empty)
		{

			(*m_tiles)(nextI, nextJ).AddDirection(OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)(startI, startJ).AddDirection(DIRECTIONS[index]);
			id = SetIDManagerSingleton::Instance().GetCurrentSetID();
			(*m_tiles)(nextI, nextJ).SetType(TileType::Passage);
			(*m_tiles)(nextI, nextJ).SetID(id);
			CarvePassageFull(nextI, nextJ);
		}

	}	
}


void MARecursiveBacktracker::GenerateByStep()
{
	CanGenerate();
	SetDoneState(false);
	int startI = 0, startJ = 0;
	int id;
	while (startI < m_rowCount)
	{
		startJ = 0;
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}
		while (startJ < m_columnCount)
		{
			if (!CanGenerate())
			{
				ClearGenerate();
				break;
			}
			if ((*m_tiles)(startI, startJ).GetType() == TileType::Empty)
			{
				id = SetIDManagerSingleton::Instance().GetNextSetID();
				(*m_tiles)(startI, startJ).SetType(TileType::Passage);
				(*m_tiles)(startI, startJ).SetID(id);
				CarvePassageByStep(startI, startJ);
			}

			startJ++;
		}

		startI++;
	}

	SetDoneState(true);
}

void MARecursiveBacktracker::CarvePassageByStep(int startI, int startJ)
{
	std::array<int, 4> directionIndices = { 0, 1, 2, 3 };
	std::ranges::shuffle(directionIndices, m_randomNumGen);
	int nextI, nextJ, index, id;

	for (int i = 0; i < 4; ++i)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));
		index = directionIndices[i];
		auto [deltaI, deltaJ] = DIRECTION_CHANGES[index];

		nextI = startI + deltaI;
		nextJ = startJ + deltaJ;

		if (nextI >= 0 && nextI < m_rowCount &&
			nextJ >= 0 && nextJ < m_columnCount &&
			(*m_tiles)(nextI, nextJ).GetType() == TileType::Empty)
		{
			(*m_tiles)(nextI, nextJ).AddDirection(MazeDefs::OPPOSITE_DIRECTIONS[index]);
			(*m_tiles)(startI, startJ).AddDirection(MazeDefs::DIRECTIONS[index]);
			id = SetIDManagerSingleton::Instance().GetCurrentSetID();
			(*m_tiles)(nextI, nextJ).SetType(TileType::Passage);
			(*m_tiles)(nextI, nextJ).SetID(id);

			// Notify that tiles have changed (for dirty flag optimization)
			NotifyTilesModified();

			CarvePassageByStep(nextI, nextJ);
		}

	}
}
