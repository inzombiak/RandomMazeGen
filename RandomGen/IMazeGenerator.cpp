#include "IMazeGenerator.h"

GenerateType IMazeGenerator::m_generateType;
std::atomic<bool> IMazeGenerator::m_done;
std::condition_variable IMazeGenerator::m_doneCV;
std::mutex IMazeGenerator::m_doneCVMutex;
std::atomic_flag IMazeGenerator::m_generate{ 0 };
std::array<Tile::PassageDirection, 4>	IMazeGenerator::DIRECTIONS = { Tile::East, Tile::West, Tile::North, Tile::South };
std::array<Tile::PassageDirection, 4>	IMazeGenerator::OPPOSITE_DIRECTIONS = { Tile::West, Tile::East, Tile::South, Tile::North };
std::array<std::pair<int, int>, 4>		IMazeGenerator::DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };