#ifndef MAZE_CONNECTOR_H
#define MAZE_CONNECTOR_H

#include "Tile.h"
#include "IThreadedSolver.h"

class MazeConnector : public IThreadedSolver
{

public:
	void ConnectMaze(const std::vector<MazeDefs::IntRect>& rooms, const TileHolder& tiles, const MazeDefs::GenerateType& genType, unsigned seed, int sleepDuration);

private:

	//Allows full to run faster
	void ConnectFull();
	void ConnectRoomFull(int index);

	void ConnectByStep();
	void ConnectRoomByStep(int index);

	void FloodSet(const std::pair<int, int>& indices, int id);
	void FloodSetByStep(const std::pair<int, int>& indices, int id);

	std::vector<MazeDefs::IntRect> m_rooms;
	const TileHolder* m_tiles;

	int m_rowCount;
	int m_columnCount;
	int m_seed;
	int m_sleepDuration;
	//For step generation

	MazeDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
	std::uniform_int_distribution<int> m_distribution;
};

#endif
