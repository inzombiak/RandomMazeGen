#include "MazeConnector.h"

#include <thread>
#include <set>

using namespace GameDefs;

void MazeConnector::ConnectMaze(const std::vector<sf::IntRect>& rooms, std::vector<std::vector<Tile>>& tiles, const GameDefs::GenerateType& genType, unsigned seed, int sleepDuration)
{
	m_rooms = rooms;
	m_rowCount = (int)tiles.size();
	if (m_rowCount < 1 || m_rooms.size() < 1)
		return;
	{
		std::unique_lock<std::mutex> lock;
		m_generateType = genType;
	}
	m_distribution = std::uniform_int_distribution<int>(0, 5);
	m_sleepDuration = sleepDuration;
	m_randomNumGen.seed(seed);
	m_tiles = &tiles;
	m_seed = seed;
	m_columnCount = (int)(*m_tiles)[0].size();

	if (genType == Step)
	{
		ConnectByStep();
		SetDoneState(true);
	}
	else
		ConnectFull();

}

void MazeConnector::ConnectFull()
{
	for (int i = 0; i < m_rooms.size(); ++i)
		ConnectRoomFull(i);
}

void MazeConnector::ConnectRoomFull(int index)
{
	int roomX, roomY, roomW, roomH, row, column, dirIndex;
	std::vector<std::pair<std::pair<int, int>, int>> possibleDoors;

	roomX = m_rooms[index].left;
	roomY = m_rooms[index].top;
	roomW = m_rooms[index].width;
	roomH = m_rooms[index].height;

	//Create list of possible doors
	for (column = roomX; column < roomX + roomW; ++column)
	{
		if (roomY > 0)
			possibleDoors.push_back(std::make_pair(std::make_pair(roomY, column), 2));

		if (roomY + roomH < m_rowCount - 1)
			possibleDoors.push_back(std::make_pair(std::make_pair(roomY + roomH - 1, column), 3));
	}

	for (row = roomY; row < roomY + roomH; ++row)
	{
		if (roomX > 0)
			possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX), 0));

		if (roomX + roomW < m_columnCount - 1)
			possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX + roomW), 1));;
	}

	//Shuffle it so theres some randomness
	shuffle(possibleDoors.begin(), possibleDoors.end(), m_randomNumGen);

	std::pair<int, int> tileIndices, nextTileIndices;
	int j, currID, nextID;
	//Loop until we find an unconnected region
	for (j = 0; j < possibleDoors.size(); ++j)
	{
		
		tileIndices = possibleDoors[j].first;

		dirIndex = possibleDoors[j].second;
		nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
		nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;

		currID = (*m_tiles)[tileIndices.first][tileIndices.second].GetID();
		nextID = (*m_tiles)[nextTileIndices.first][nextTileIndices.second].GetID();
		if (nextID == currID)
			continue;

		//Open it up
		(*m_tiles)[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
		(*m_tiles)[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
	//(*m_tiles)[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
		(*m_tiles)[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);

		//Flood it cause pretty
		FloodSet(nextTileIndices, (*m_tiles)[tileIndices.first][tileIndices.second].GetID());
		//Flood it cause pretty
		if (SetIDManagerSingleton::Instance().GetSetMemberCount(currID) > SetIDManagerSingleton::Instance().GetSetMemberCount(nextID))
			FloodSet(nextTileIndices, currID);
		else
			FloodSet(tileIndices, nextID);
		//Done
		break;
	}

	//Now we clear the possible doors, with a small chance of opening them up if theyre valid
	//TODO: CONTINUING MIGHT CAUSE ISSUES
	for (; j < possibleDoors.size(); ++j)
	{
		tileIndices = possibleDoors[j].first;

		dirIndex = possibleDoors[j].second;
		nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
		nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;
		currID = (*m_tiles)[tileIndices.first][tileIndices.second].GetID();
		nextID = (*m_tiles)[nextTileIndices.first][nextTileIndices.second].GetID();

		if (m_distribution(m_randomNumGen) == 0 ||
			nextID != currID)
		{

			//Open it up
			(*m_tiles)[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
			(*m_tiles)[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
			//(*m_tiles)[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
			(*m_tiles)[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
			//Flood it cause pretty
			if (SetIDManagerSingleton::Instance().GetSetMemberCount(currID) > SetIDManagerSingleton::Instance().GetSetMemberCount(nextID))
				FloodSet(nextTileIndices, currID);
			else
				FloodSet(tileIndices, nextID);
			
		}
	}
}

void MazeConnector::FloodSet(const std::pair<int, int>& indices, int id)
{
	if (indices.first < 0 || indices.first >= m_rowCount
		|| indices.second < 0 || indices.second >= m_columnCount || (*m_tiles)[indices.first][indices.second].GetID() == id)
		return;

	std::pair<int, int> nextIndices;
	(*m_tiles)[indices.first][indices.second].SetID(id);
	(*m_tiles)[indices.first][indices.second].SetColor(SetIDManagerSingleton::Instance().GetSetColor(id, m_seed));

	for (int i = 0; i < 4; ++i)
	{
		if ((*m_tiles)[indices.first][indices.second].HasDirection(DIRECTIONS[i]))
		{
			nextIndices.first = indices.first + DIRECTION_CHANGES[i].first;
			nextIndices.second = indices.second + DIRECTION_CHANGES[i].second;

			FloodSet(nextIndices, id);
		}
	}
}

void MazeConnector::ConnectByStep()
{
	CanGenerate();

	SetDoneState(false);

	for (int i = 0; i < m_rooms.size(); ++i)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}
		ConnectRoomByStep(i);
	}


	SetDoneState(true);
}

void MazeConnector::ConnectRoomByStep(int index)
{
	int roomX, roomY, roomW, roomH, row, column, dirIndex;
	std::vector<std::pair<std::pair<int, int>, int>> possibleDoors;

	roomX = m_rooms[index].left;
	roomY = m_rooms[index].top;
	roomW = m_rooms[index].width;
	roomH = m_rooms[index].height;

	//Create list of possible doors
	for (column = roomX; column < roomX + roomW; ++column)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		if (roomY > 0)
			possibleDoors.push_back(std::make_pair(std::make_pair(roomY, column), 2));

		if (roomY + roomH < m_rowCount - 1)
			possibleDoors.push_back(std::make_pair(std::make_pair(roomY + roomH - 1, column), 3));
	}

	for (row = roomY; row < roomY + roomH; ++row)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		if (roomX > 0)
			possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX), 0));

		if (roomX + roomW < m_columnCount - 1)
			possibleDoors.push_back(std::make_pair(std::make_pair(row, roomX + roomW), 1));;
	}

	//Shuffle it so theres some randomness
	shuffle(possibleDoors.begin(), possibleDoors.end(), m_randomNumGen);

	std::pair<int, int> tileIndices, nextTileIndices;
	int j, currID, nextID;
	//Loop until we find an unconnected region
	for (j = 0; j < possibleDoors.size(); ++j)
	{

		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		tileIndices = possibleDoors[j].first;

		dirIndex = possibleDoors[j].second;
		nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
		nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;
		currID = (*m_tiles)[tileIndices.first][tileIndices.second].GetID();
		nextID = (*m_tiles)[nextTileIndices.first][nextTileIndices.second].GetID();

		if (nextID == currID)
			continue;

		//Open it up
		(*m_tiles)[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
		(*m_tiles)[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
		(*m_tiles)[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
		(*m_tiles)[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);

		//Flood it cause pretty
		if (SetIDManagerSingleton::Instance().GetSetMemberCount(currID) > SetIDManagerSingleton::Instance().GetSetMemberCount(nextID))
			FloodSetByStep(nextTileIndices, currID);
		else
			FloodSetByStep(tileIndices, nextID);

		//Done
		break;
	}
	
	//Now we clear the possible doors, with a small chance of opening them up if theyre valid
	//TODO: CONTINUING MIGHT CAUSE ISSUES
	for (; j < possibleDoors.size(); ++j)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			break;
		}

		tileIndices = possibleDoors[j].first;

		dirIndex = possibleDoors[j].second;
		nextTileIndices.first = tileIndices.first + DIRECTION_CHANGES[dirIndex].first;
		nextTileIndices.second = tileIndices.second + DIRECTION_CHANGES[dirIndex].second;
		currID = (*m_tiles)[tileIndices.first][tileIndices.second].GetID();
		nextID = (*m_tiles)[nextTileIndices.first][nextTileIndices.second].GetID();
		if (m_distribution(m_randomNumGen) == 0 ||
			nextID != currID)
		{

			//Open it up
			//(*m_tiles)[tileIndices.first][tileIndices.second].AddDirection(GameDefs::DIRECTIONS[dirIndex]);
			//(*m_tiles)[nextTileIndices.first][nextTileIndices.second].AddDirection(GameDefs::OPPOSITE_DIRECTIONS[dirIndex]);
			(*m_tiles)[tileIndices.first][tileIndices.second].SetBorder(GameDefs::DIRECTIONS[dirIndex], 4, sf::Color::Yellow);
			(*m_tiles)[nextTileIndices.first][nextTileIndices.second].SetBorder(GameDefs::OPPOSITE_DIRECTIONS[dirIndex], 4, sf::Color::Yellow);

			//Flood it cause pretty
			if (SetIDManagerSingleton::Instance().GetSetMemberCount(currID) > SetIDManagerSingleton::Instance().GetSetMemberCount(nextID))
				FloodSetByStep(nextTileIndices, currID);
			else
				FloodSetByStep(tileIndices, nextID);
		}
	}
}

void MazeConnector::FloodSetByStep(const std::pair<int, int>& indices, int id)
{
	if (!CanGenerate())
	{
		ClearGenerate();
		return;
	}

	if (indices.first < 0 || indices.first >= m_rowCount
		|| indices.second < 0 || indices.second >= m_columnCount || (*m_tiles)[indices.first][indices.second].GetID() == id)
		return;

	std::pair<int, int> nextIndices;
	(*m_tiles)[indices.first][indices.second].SetID(id);
	(*m_tiles)[indices.first][indices.second].SetColor(SetIDManagerSingleton::Instance().GetSetColor(id, m_seed));

	for (int i = 0; i < 4; ++i)
	{
		if (!CanGenerate())
		{
			ClearGenerate();
			return;
		}

		if ((*m_tiles)[indices.first][indices.second].HasDirection(DIRECTIONS[i]))
		{
			nextIndices.first = indices.first + DIRECTION_CHANGES[i].first;
			nextIndices.second = indices.second + DIRECTION_CHANGES[i].second;

			std::this_thread::sleep_for(std::chrono::milliseconds(m_sleepDuration));

			FloodSetByStep(nextIndices, id);
		}
	}
}
