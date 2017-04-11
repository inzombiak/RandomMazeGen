#include <iostream>

#include "GridManager.h"

#include <iostream>

const int WINDOW_HEIGHT = 800;
const int WINDOW_WIDTH = 800;
int main()
{
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Random Gen");
	GridManager gm;
	gm.GenerateMap(WINDOW_WIDTH, WINDOW_HEIGHT, 30, 30);

	std::cout << "R - Generate New Map" << std::endl;
	std::cout << "G - Toggle Maze Generator(Recursive Backtacker(default) and Eller's Algortihm)" << std::endl;
	std::cout << "T - Toggle between watch and instant (instant is default)" << std::endl;

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				gm.Close();
				window.close();
			}
			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.code == sf::Keyboard::R)
					gm.RandomizeMap();
				else if (event.key.code == sf::Keyboard::G)
					gm.ToggleMazeGenerator();
				else if (event.key.code == sf::Keyboard::T)
					gm.ToggleMazeGenerateType();
			}
		}

		window.clear(sf::Color::White);
		gm.Draw(window);
		window.display();
	}

	return 0;
}