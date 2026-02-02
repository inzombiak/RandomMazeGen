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
	const TileHolder* m_tiles = nullptr;

	int m_rowCount = 0;
	int m_columnCount = 0;
	int m_seed = 0;
	int m_sleepDuration = 0;
	//For step generation

	MazeDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
	std::uniform_int_distribution<int> m_distribution;
};

#endif
