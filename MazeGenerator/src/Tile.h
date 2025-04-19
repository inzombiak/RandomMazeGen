#ifndef TILE_H
#define TILE_H

#include "MazeGenDefs.h"
#include "MazeGenPriv.h"

#include<map>

class Tile
{

public:
	Tile();
	
	MazeDefs::PassageDirection GetDirection() const;
	void SetDirection(const MazeDefs::PassageDirection& dir);
	void AddDirection(const MazeDefs::PassageDirection& dir);
	void RemoveDirection(const MazeDefs::PassageDirection& dir);
	bool HasDirection(const MazeDefs::PassageDirection& dir) const;
	MazeDefs::PassageDirection GetPassageDirections() const;
	std::vector<int> GetPassageDirectionIndices();

	MazeDefs::TileType GetType() const;
	void SetType(const MazeDefs::TileType& type);
	MazeDefs::Vector2f GetPosition() const;
	void SetPosition(const MazeDefs::Vector2f& newPosition);
	void SetID(int newID);
	int GetID() const;


	void Reset();

private:
	int m_id;

	MazeDefs::PassageDirection m_direction;
	MazeDefs::TileType m_type;
	MazeDefs::Vector2f m_position;
};

class TileHolder {
public:
	void Reset(int rowCount, int columnCount) {
		m_rowCount = rowCount;
		m_columnCount = columnCount;

		if (m_data != NULL)
			delete[] m_data;

		m_data = new Tile[m_rowCount * m_columnCount];
	};
	Tile* GetData() {
		return m_data;
	}
	int GetRowCount() const {
		return m_rowCount;
	}
	int GetColumnCount() const {
		return m_columnCount;
	};

	Tile& operator()(int i, int j) const {
		return m_data[i * m_columnCount + j];
	};

private:
	int m_rowCount;
	int m_columnCount;
	Tile* m_data;
};

#endif