#include "DeadEndRemover.h"

using namespace GameDefs;

void DeadEndRemover::RemoveDeadEnds(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, int removalPercentage, unsigned seed, int sleepDuration)
{
	m_rowCount = tiles.size();
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
	m_columnCount = (*m_tiles)[0].size();
	m_removalPercentage = removalPercentage;
	if (genType == Step)
	{
		RemoveByStep();
		SetDoneState(true);
	}
	else
		RemoveFull();
	

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
	if ((*m_tiles)[i][j].GetType() != TileType::Passage)
		return;

	tilePassageIndices = (*m_tiles)[i][j].GetPassageDirectionIndices();
	
	//Only want dead ends (only 1 passage is open)
	if (tilePassageIndices.size() != 1)
		return;

	(*m_tiles)[i][j].SetType(TileType::Empty);
	(*m_tiles)[i][j].SetColor(sf::Color::White);

	nextI = i + DIRECTION_CHANGES[tilePassageIndices[0]].first;
	nextJ = j + DIRECTION_CHANGES[tilePassageIndices[0]].second;

	if (nextI < 0 || nextI >= m_rowCount ||
		nextJ < 0 || nextJ >= m_columnCount)
		return;

	(*m_tiles)[nextI][nextJ].RemoveDirection(OPPOSITE_DIRECTIONS[tilePassageIndices[0]]);

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
	if ((*m_tiles)[i][j].GetType() != TileType::Passage)
		return;

	tilePassageIndices = (*m_tiles)[i][j].GetPassageDirectionIndices();

	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	//Only want dead ends (only 1 passage is open)
	if (tilePassageIndices.size() != 1)
		return;

	(*m_tiles)[i][j].SetType(TileType::Empty);
	(*m_tiles)[i][j].SetColor(sf::Color::White);

	nextI = i + DIRECTION_CHANGES[tilePassageIndices[0]].first;
	nextJ = j + DIRECTION_CHANGES[tilePassageIndices[0]].second;

	if (nextI < 0 || nextI >= m_rowCount ||
		nextJ < 0 || nextJ >= m_columnCount)
		return;

	(*m_tiles)[nextI][nextJ].RemoveDirection(OPPOSITE_DIRECTIONS[tilePassageIndices[0]]);
	std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	RemoveDeadEndByStep(nextI, nextJ);
}