#include "DeadEndRemover.h"

using namespace MazeDefs;

void DeadEndRemover::RemoveDeadEnds(const TileHolder& tiles, const MazeDefs::GenerateType& genType, int removalPercentage, unsigned seed, int sleepDuration)
{
	m_rowCount = tiles.GetRowCount();
	if (m_rowCount < 1)
		return;
	{
		std::unique_lock<std::mutex> lock;
		m_generateType = genType;
	}
	m_distribution = std::uniform_int_distribution<int>(0, 100);
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_seed = seed;
	m_columnCount = tiles.GetColumnCount();
	m_removalPercentage = removalPercentage;
	if (genType == Step)
		RemoveByStep();
	else
		RemoveFull();
	SetDoneState(true);
}

void DeadEndRemover::RemoveFull()
{
	for (int i = 0; i < m_rowCount; ++i)
	{
		for (int j = 0; j < m_columnCount; j++)
		{
			if (m_distribution(m_randomNumGen) > m_removalPercentage)
				continue;

			RemoveDeadEnd(i, j);
		}
	}
}

void DeadEndRemover::RemoveDeadEnd(int i, int j)
{
	std::vector<int> tilePassageIndices;
	int nextI, nextJ;
	
	//Skip rooms and empty tiles
	if ((*m_tiles)(i ,j).GetType() != TileType::Passage)
		return;

	tilePassageIndices = (*m_tiles)(i, j).GetPassageDirectionIndices();
	
	//Only want dead ends (only 1 passage is open)
	if (tilePassageIndices.size() != 1)
		return;

	(*m_tiles)(i, j).SetType(TileType::Empty);

	auto [deltaI, deltaJ] = DIRECTION_CHANGES[tilePassageIndices[0]];

	nextI = i + deltaI;
	nextJ = j + deltaJ;

	if (nextI < 0 || nextI >= m_rowCount ||
		nextJ < 0 || nextJ >= m_columnCount)
		return;

	(*m_tiles)(nextI, nextJ).RemoveDirection(OPPOSITE_DIRECTIONS[tilePassageIndices[0]]);
	NotifyTilesModified();

	RemoveDeadEnd(nextI, nextJ);
}


void DeadEndRemover::RemoveByStep()
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

		for (int j = 0; j < m_columnCount; j++)
		{
			if (!CanGenerate())
			{
				ClearGenerate();
				break;
			}
			if (m_distribution(m_randomNumGen) > m_removalPercentage)
				continue;

			RemoveDeadEndByStep(i, j);
		}
	}

	SetDoneState(true);
}

void DeadEndRemover::RemoveDeadEndByStep(int i, int j)
{
	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	std::vector<int> tilePassageIndices;
	int nextI, nextJ;
	//Skip rooms and empty tiles
	if ((*m_tiles)(i, j).GetType() != TileType::Passage)
		return;

	tilePassageIndices = (*m_tiles)(i, j).GetPassageDirectionIndices();

	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	//Only want dead ends (only 1 passage is open)
	if (tilePassageIndices.size() != 1)
		return;

	(*m_tiles)(i, j).SetType(TileType::Empty);

	auto [deltaI, deltaJ] = DIRECTION_CHANGES[tilePassageIndices[0]];

	nextI = i + deltaI;
	nextJ = j + deltaJ;

	if (nextI < 0 || nextI >= m_rowCount ||
		nextJ < 0 || nextJ >= m_columnCount)
		return;

	(*m_tiles)(nextI, nextJ).RemoveDirection(OPPOSITE_DIRECTIONS[tilePassageIndices[0]]);
	NotifyTilesModified();
	std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	RemoveDeadEndByStep(nextI, nextJ);
}