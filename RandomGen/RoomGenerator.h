#ifndef ROOM_GENERATOR_H
#define ROOM_GENERATOR_H

#include "Tile.h"

#include <random>  
#include <atomic>
#include <condition_variable>

class RoomGenerator
{

public:

	void SetPlacementAttemptCount(int count);
	void SetRoomHorizontalBounds(const sf::Vector2i& newHorizBounds);
	sf::Vector2i GetRoomHorizontalBounds();
	void SetRoomVerticalBounds(const sf::Vector2i& newVertBounds);
	sf::Vector2i GetRoomVerticalBounds();
	
	const std::vector<sf::IntRect>& GenerateRoom(std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration);

private:

	//Allows full to run faster
	void GenerateFull();
	void GenerateByStep();

	//TODO: May need to lock all reads for these if I add GUI
	sf::Vector2i m_verticalBounds;
	sf::Vector2i m_horizontalBounds;
	int m_attemptCount;

	std::vector<sf::IntRect> m_rooms;
	std::vector<std::vector<Tile>>* m_tiles;
	int m_rowCount;
	int m_columnCount;
	//For step generation
	//TODO: STATIC MAY CAUSE ISSUES
	static std::atomic_flag m_generate;
	static std::condition_variable m_doneCV;
	static std::mutex m_doneCVMutex;
	std::mutex m_horizontalMutex;
	std::mutex m_verticalMutex;
	std::mutex m_attemptMutex;
	static std::atomic<bool> m_done;
	int m_sleepDuration;

	GameDefs::GenerateType m_generateType;
	std::default_random_engine m_randomNumGen;

};

#endif
