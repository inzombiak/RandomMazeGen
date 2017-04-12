#ifndef MAZE_CONNECTOR_H
#define MAZE_CONNECTOR_H

#include "Tile.h"
#include "IThreadedSolver.h"

class MazeConnector : public IThreadedSolver
{

public:

	void ConnectMaze(const std::vector<sf::IntRect>& rooms, std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration);
private:

	//Allows full to run faster
	void ConnectFull();
	void ConnectRoomFull(int index);

	void ConnectByStep();
	void ConnectRoomByStep(int index);

	void FloodSet(const std::pair<int, int>& indices, int id);
	void FloodSetByStep(const std::pair<int, int>& indices, int id);

	std::vector<sf::IntRect> m_rooms;
	std::vector<std::vector<Tile>>* m_tiles;

	int m_rowCount;
	int m_columnCount;
	int m_seed;
	int m_sleepDuration;
	//For step generation
	//TODO: STATIC MAY CAUSE ISSUES


	GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
	std::uniform_int_distribution<int> m_distribution;
};

#endif
