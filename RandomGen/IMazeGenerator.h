#ifndef I_MAZE_GENERATOR_H
#define I_MAZE_GENERATOR_H

#include "Tile.h"

#include <array>
#include <random>  
#include <atomic>
#include <condition_variable>

class IMazeGenerator
{
public:
	virtual ~IMazeGenerator() {};

	virtual void GenerateMaze(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration) = 0;
	void TerminateGeneration()
	{
		{
			std::unique_lock<std::mutex> lock(m_generatingMutex);
			if (!m_generating)
				return;
		}

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

protected:
	IMazeGenerator(){}
	//Allows full to run faster
	virtual void GenerateFull() = 0;
	virtual void GenerateByStep() = 0;

	int m_rowCount;
	int m_columnCount;
	std::vector<std::vector<Tile>>* m_tiles;

	//For step generation
	//TODO: STATIC MAY CAUSE ISSUES
	static std::atomic_flag m_generate;
	static std::condition_variable m_doneCV;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
	static std::mutex m_doneCVMutex;
	static std::mutex m_generatingMutex;
	static std::atomic<bool> m_done;
	static bool m_generating;
	int m_sleepDuration;

	static GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;
};


#endif