#ifndef DEAD_END_REMOVER_H
#define DEAD_END_REMOVER_H

#include "Tile.h"
#include "IThreadedSolver.h"

class DeadEndRemover : public IThreadedSolver
{
public:
	
	void RemoveDeadEnds(const TileHolder& tiles, const MazeDefs::GenerateType& genType, int removalPercentage, unsigned seed, int sleepDuration);

private:

	void RemoveFull();
	void RemoveDeadEnd(int i, int j);
	void RemoveByStep();
	void RemoveDeadEndByStep(int i, int j);

	const TileHolder* m_tiles;

	int m_rowCount = 0;
	int m_columnCount = 0;
	int m_seed = 0;
	int m_sleepDuration = 0;
	int m_removalPercentage = 0;

	MazeDefs::GenerateType m_generateType = MazeDefs::GenerateType::Full;
	std::default_random_engine m_randomNumGen;
	std::uniform_int_distribution<int> m_distribution;
};

#endif
