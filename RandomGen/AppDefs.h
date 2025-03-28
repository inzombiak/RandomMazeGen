#ifndef APPDEFS_H
#define APPDEFS_H

#include <map>
#include <memory>

class Renderer_D12;
class Window;
namespace Globals {

	struct StartupValues {
		int window_width = 1200;
		int window_height = 800;
		const wchar_t* window_className = L"DX12WindowClass";
		bool use_warp = false;
	};

	extern bool VSYNC_ENABLED;

	extern StartupValues STARTUP_VALS;

	struct InputState {
		std::pair<int, int> mousePos;
		int mouseBtnState = 0;
		bool keyStates[256] = { false };

		std::pair<int, int> lastMouseDownPos = std::pair<int, int>(0, 0);
	};

	extern InputState INPUT_STATE;

	static const float CAM_PAN_SPEED = 8.f;
	static const float CAM_ROT_SPEED = 0.01f;
}

extern std::shared_ptr<Window>		 GAME_WINDOW;
extern std::shared_ptr<Renderer_D12> RENDERER;

#endif