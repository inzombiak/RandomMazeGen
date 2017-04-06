#ifndef MG_ELLERS_H
#define MG_ELLERS_H

#include "IMazeGenerator.h"

#include <set>
#include <map>

class MGEllers : public IMazeGenerator
{
public:
	void GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GenerateType& genType, unsigned seed, int sleepDuration) override;
protected:

	void GenerateFull() override;
	void GenerateByStep() override;

private:
	//For GenerateFull
	void InitalizeRow(int row);
	void MergeColumns(int row);
	void MakeVerticalCuts(int row);
	/*
		This was my first attempt at a multithreaded app like this
		I made separate functions for each of the possible evocations
		since full doesn’t need to check any multithreaded stuff
		I don’t know if this is the "right" way to do it.
	*/
	//For GenerateByStep
	void InitalizeRowByStep(int row);
	void MergeColumnsByStep(int row);
	void MakeVerticalCutsByStep(int row);

	bool CompareAndMergeSets(const std::pair<int, int>& first, const std::pair<int, int>& second);

	int m_rowCount;
	int m_columnCount;
	int m_lastSet;
	//Keeps track of the rows sets
	std::vector<std::pair<std::pair<int, int>, int>> m_rowSets;

	//Set number to tile indices
	std::map<int, std::vector<std::pair<int, int>>> m_setToIndices;
	typedef std::map<int, std::vector<std::pair<int, int>>>::iterator sToIIterator;

};

#endif