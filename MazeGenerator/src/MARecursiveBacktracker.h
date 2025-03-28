#pragma once
#include "IMazeAlgorithm.h"
class MARecursiveBacktracker :
	public IMazeAlgorithm
{
public:
	void GenerateMaze(const TileHolder& tiles, const MazeDefs::GenerateType& genType, unsigned seed, int sleepDuration) override;

protected:
	void GenerateFull() override;
	void GenerateByStep() override;

private:

	void CarvePassageFull(int startI, int startJ);
	void CarvePassageByStep(int startI, int startJ);
};

