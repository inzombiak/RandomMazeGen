#include <iostream>

#include "GridManager.h"

const int WINDOW_HEIGHT = 800;
const int WINDOW_WIDTH = 800;


int main()
{
	sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "GJK");
	GridManager gm;
	gm.GenerateMap(WINDOW_WIDTH, WINDOW_HEIGHT, 20, 20);
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
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