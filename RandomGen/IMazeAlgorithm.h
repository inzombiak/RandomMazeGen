#ifndef I_MAZE_GENERATOR_H
#define I_MAZE_GENERATOR_H

#include "Tile.h"
#include "IThreadedSolver.h"

class IMazeAlgorithm : public IThreadedSolver
{
public:
	virtual ~IMazeAlgorithm() {};

	virtual void GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration) = 0;
protected:
	IMazeAlgorithm(){}
	//Allows full to run faster
	virtual void GenerateFull() = 0;
	virtual void GenerateByStep() = 0;

	int m_rowCount;
	int m_columnCount;
	std::vector<std::vector<Tile>>* m_tiles;
	int m_seed;
	int m_sleepDuration;

	GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
};


#endif