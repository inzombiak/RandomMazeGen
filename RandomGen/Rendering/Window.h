#ifndef WINDOW_H
#define WINDOW_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif


#if defined(CreateWindow)
#undef CreateWindow
#endif

#include "../GameDefs.h"
#include "../Events.h"
#include <assert.h>

class App;
class Window {
	public:
		Window(HINSTANCE hInstance);

		void ToggleFullscreen();
		void Show();
		void Close();
		void RegisterCallbacks(std::shared_ptr<App> app);
		HWND GetHandle() const;
		
		RECT GetRect() const;
		int GetWidth() const;
		int GetHeight() const;

		void OnUpdate(UpdateEventArgs& e);
		void OnRender(RenderEventArgs& e);

		void OnKeyPressed(KeyEventArgs& e);
		void OnKeyReleased(KeyEventArgs& e);

		void OnMouseMoved(MouseMotionEventArgs& e);
		void OnMouseButtonPressed(MouseButtonEventArgs& e);
		void OnMouseButtonReleased(MouseButtonEventArgs& e);
		void OnMouseWheel(MouseWheelEventArgs& e);

		void OnResize(ResizeEventArgs& e);

private:
		Window(const Window&) = delete;

		HWND m_handle;
		RECT m_rect;

		int m_width  = 1200;
		int m_height = 800;

		bool m_fullscreen = false;

		std::shared_ptr<App> m_app;
};

#endif;