#include "IMazeGenerator.h"

std::mutex IMazeGenerator::m_generatingMutex;
bool IMazeGenerator::m_generating = false;
GameDefs::GenerateType IMazeGenerator::m_generateType;
std::atomic<bool> IMazeGenerator::m_done;
std::condition_variable IMazeGenerator::m_doneCV;
std::mutex IMazeGenerator::m_doneCVMutex;
std::atomic_flag IMazeGenerator::m_generate{ 0 };
std::array<GameDefs::PassageDirection, 4>	IMazeGenerator::DIRECTIONS = { GameDefs::East, GameDefs::West, GameDefs::North, GameDefs::South };
std::array<GameDefs::PassageDirection, 4>	IMazeGenerator::OPPOSITE_DIRECTIONS = { GameDefs::West, GameDefs::East, GameDefs::South, GameDefs::North };
std::array<std::pair<int, int>, 4>		IMazeGenerator::DIRECTION_CHANGES = { std::make_pair(0, 1), std::make_pair(0, -1), std::make_pair(-1, 0), std::make_pair(1, 0) };