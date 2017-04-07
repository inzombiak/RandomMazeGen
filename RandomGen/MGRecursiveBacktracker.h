#pragma once
#include "IMazeGenerator.h"
class MGRecursiveBacktracker :
	public IMazeGenerator
{
public:
	void GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration) override;

protected:
	void GenerateFull() override;
	void GenerateByStep() override;

private:

	void CarvePassageFull(int startI, int startJ);
	void CarvePassageByStep(int startI, int startJ);
};

