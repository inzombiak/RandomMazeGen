#ifndef TILE_H
#define TILE_H

#include <SFML\Window.hpp>
#include <SFML\Graphics.hpp>

#include "GameDefs.h"

#include<map>

class Tile
{

public:
	Tile(const sf::RectangleShape& tile, const GameDefs::TileType& type, int borderWidth, const sf::Color& borderColor);
	
	GameDefs::PassageDirection GetDirection() const;
	void SetDirection(const GameDefs::PassageDirection& dir);
	void AddDirection(const GameDefs::PassageDirection& dir);
	void RemoveDirection(const GameDefs::PassageDirection& dir);
	bool HasDirection(const GameDefs::PassageDirection& dir) const;
	std::vector<int> GetPassageDirectionIndices();
	GameDefs::TileType GetType() const;
	void SetType(const GameDefs::TileType& type);
	sf::Vector2f GetPosition() const;
	void SetPosition(const sf::Vector2f& newPosition);
	void SetID(int newID);
	int GetID() const;
	void SetColor(const sf::Color& color);
	void SetBorder(const GameDefs::PassageDirection& border, int borderWidth, const sf::Color& color);

	void Reset();

	void Draw(sf::RenderWindow& rw);

private:
	int m_id;

	sf::RectangleShape m_tile;

	//TODO: TOO BRUTE FORCE
	std::map<GameDefs::PassageDirection, std::pair<int, sf::Color>> m_borderInfo;

	std::pair<int, sf::Color> m_defaultBorderInfo;
	GameDefs::PassageDirection m_direction;
	GameDefs::TileType m_type;
};


#endif