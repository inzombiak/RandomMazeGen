#ifndef DEAD_END_REMOVER_H
#define DEAD_END_REMOVER_H

#include "Tile.h"
#include "IThreadedSolver.h"

class DeadEndRemover : public IThreadedSolver
{
public:
	
	void RemoveDeadEnds(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, int removalPercentage, unsigned seed, int sleepDuration);

private:

	void RemoveFull();
	void RemoveDeadEnd(int i, int j);
	void RemoveByStep();
	void RemoveDeadEndByStep(int i, int j);

	std::vector<std::vector<Tile>>* m_tiles;

	int m_rowCount;
	int m_columnCount;
	int m_seed;
	int m_sleepDuration;
	int m_removalPercentage;

	GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
	std::uniform_int_distribution<int> m_distribution;

};

#endif
