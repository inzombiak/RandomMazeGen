#include "IMazeGenerator.h"

std::mutex IMazeGenerator::m_generatingMutex;
bool IMazeGenerator::m_generating = false;
GameDefs::GenerateType IMazeGenerator::m_generateType;
std::atomic<bool> IMazeGenerator::m_done;
std::condition_variable IMazeGenerator::m_doneCV;
std::mutex IMazeGenerator::m_doneCVMutex;
std::atomic_flag IMazeGenerator::m_generate{ 0 };
