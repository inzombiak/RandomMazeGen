#ifndef ROOM_GENERATOR_H
#define ROOM_GENERATOR_H

#include "Tile.h"

#include <random>  
#include <atomic>
#include <condition_variable>

class RoomGenerator
{

public:
	RoomGenerator() {};

	void SetPlacementAttemptCount(int count);
	void SetRoomHorizontalBounds(const MazeDefs::Vector2i& newHorizBounds);
	MazeDefs::Vector2i GetRoomHorizontalBounds();
	void SetRoomVerticalBounds(const MazeDefs::Vector2i& newVertBounds);
	MazeDefs::Vector2i GetRoomVerticalBounds();
	
	const std::vector<MazeDefs::IntRect>& GenerateRoom(const TileHolder& tiles, const MazeDefs::GenerateType& genType, unsigned seed, int sleepDuration);

private:

	//Allows full to run faster
	void GenerateFull();
	void GenerateByStep();

	//TODO: May need to lock all reads for these if I add GUI
	MazeDefs::Vector2i m_verticalBounds;
	MazeDefs::Vector2i m_horizontalBounds;
	int m_attemptCount = 0;

	std::vector<MazeDefs::IntRect> m_rooms;
	const TileHolder* m_tiles = nullptr;
	int m_rowCount = 0;
	int m_columnCount = 0;
	int m_seed = 0;
	//For step generation
	//TODO: STATIC MAY CAUSE ISSUES
	static std::atomic_flag m_generate;
	static std::condition_variable m_doneCV;
	static std::mutex m_doneCVMutex;
	std::mutex m_horizontalMutex;
	std::mutex m_verticalMutex;
	std::mutex m_attemptMutex;

	static std::atomic<bool> m_done;
	int m_sleepDuration = 0;

	MazeDefs::GenerateType m_generateType = MazeDefs::GenerateType::Full;
	std::default_random_engine m_randomNumGen;

};

#endif
