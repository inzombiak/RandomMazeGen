#ifndef I_MAZE_GENERATOR_H
#define I_MAZE_GENERATOR_H

#include "Tile.h"
#include "IThreadedSolver.h"

class IMazeAlgorithm : public IThreadedSolver
{
public:
	virtual ~IMazeAlgorithm() {};

	virtual void GenerateMaze(const TileHolder& tiles, const MazeDefs::GenerateType& genType, unsigned seed, int sleepDuration) = 0;

protected:
	//Allows full to run faster
	virtual void GenerateFull() = 0;
	virtual void GenerateByStep() = 0;

	int m_rowCount = 0;
	int m_columnCount = 0;
	const TileHolder* m_tiles = nullptr;
	int m_seed = 0;
	int m_sleepDuration = 0;

	MazeDefs::GenerateType m_generateType = MazeDefs::GenerateType::Full;
	std::default_random_engine m_randomNumGen;
};


#endif