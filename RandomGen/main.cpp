#include "Rendering/Window.h"

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
			Globals::STARTUP_VALS.window_height = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-warp") == 0 || ::wcscmp(argv[i], L"--warp") == 0)
		{
			Globals::STARTUP_VALS.use_warp = true;
		}
		if (::wcscmp(argv[i], L"-physicstest") == 0 || ::wcscmp(argv[i], L"--physicstest") == 0)
		{
			Globals::STARTUP_VALS.physics_test = true;
		}
		if (::wcscmp(argv[i], L"-physicsheadless") == 0)
		{
			Globals::STARTUP_VALS.physics_test = true;
			Globals::STARTUP_VALS.physics_headless = true;
		}
		if (::wcscmp(argv[i], L"-physicsfixed") == 0)
		{
			Globals::STARTUP_VALS.physics_test = true;
			Globals::STARTUP_VALS.physics_fixed = true;
		}
		if (::wcscmp(argv[i], L"-physicssteps") == 0)
		{
			Globals::STARTUP_VALS.physics_test = true;
			Globals::STARTUP_VALS.physics_fixed = true;
			Globals::STARTUP_VALS.physics_steps = ::wcstol(argv[++i], nullptr, 10);
		}
		if (::wcscmp(argv[i], L"-physicstraceevery") == 0)
		{
			Globals::STARTUP_VALS.physics_trace_every = ::wcstol(argv[++i], nullptr, 10);
		}
	}

	// Free memory allocated by CommandLineToArgvW
	::LocalFree(argv);
}

#include "App.h"
Globals::StartupValues Globals::STARTUP_VALS;
bool Globals::VSYNC_ENABLED = true;
int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	// Windows 10 Creators update adds Per Monitor V2 DPI awareness context.
	// Using this awareness context allows the client area of the window 
	// to achieve 100% scaling while still allowing non-client window content to 
	// be rendered in a DPI sensitive fashion.
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	// Window class name. Used for registering / creating the window.
	ParseCommandLineArguments();
	
	std::shared_ptr<App> mazeGen = std::make_shared<App>(L"MazeGen", Globals::STARTUP_VALS.window_width, Globals::STARTUP_VALS.window_height, Globals::VSYNC_ENABLED, hInstance);
	mazeGen->Initialize();
	mazeGen->Destroy();
	return 0;
}