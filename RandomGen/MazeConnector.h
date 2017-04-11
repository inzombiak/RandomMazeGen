#ifndef MAZE_CONNECTOR_H
#define MAZE_CONNECTOR_H

#include "Tile.h"

#include <random>  
#include <atomic>
#include <condition_variable>

class MazeConnector
{

public:

	void ConnectMaze(const std::vector<sf::IntRect>& rooms, std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration);
	void TerminateGeneration()
	{
		{
			std::unique_lock<std::mutex> lock(m_doneCVMutex);
			if (m_generateType != GameDefs::Step)
				return;
		}

		m_generate.clear();

		std::unique_lock<std::mutex> lock(m_doneCVMutex);
		auto not_paused = [this](){return m_done == true; };
		m_doneCV.wait(lock, not_paused);
	}

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
	//For step generation
	//TODO: STATIC MAY CAUSE ISSUES
	std::atomic_flag m_generate;
	std::condition_variable m_doneCV;
	std::mutex m_doneCVMutex;
	std::atomic<bool> m_done;
	std::mutex m_generatingMutex;
	bool m_generating;
	int m_sleepDuration;

	GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
};

#endif
