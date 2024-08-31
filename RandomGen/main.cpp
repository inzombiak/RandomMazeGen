#include "GameDefs.h"

#if USE_SFML
#include <iostream>

#include "GridManager.h"

#include <iostream>

int main()
{
	sf::RenderWindow window(sf::VideoMode(800, 800), "Random Gen");

	GridManager gm;
	gm.GenerateMap(800, 800, 40, 40);

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

#else
#include "Rendering/Window.h"
#include "GameDefs.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>

void ParseCommandLineArguments()
{
	int argc;
	wchar_t** argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);

	for (size_t i = 0; i < argc; ++i)
	{
		if (::wcscmp(argv[i], L"-w") == 0 || ::wcscmp(argv[i], L"--width") == 0)
		{
			Globals::STARTUP_VALS.window_width = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-h") == 0 || ::wcscmp(argv[i], L"--height") == 0)
		{
			Globals::STARTUP_VALS.window_width = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			Globals::STARTUP_VALS.window_width = true;
		}
	}

	// Free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

#include "MazeGenApp.h"
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Window class name. Used for registering / creating the window.
	ParseCommandLineArguments();
	
	std::shared_ptr<MazeGenApp> mazeGen = std::make_shared<MazeGenApp>(L"MazeGen", Globals::STARTUP_VALS.window_width, Globals::STARTUP_VALS.window_height, Globals::VSYNC_ENABLED, hInstance);
	mazeGen->Initialize();
	mazeGen->Destroy();
	return 0;
}
#endif