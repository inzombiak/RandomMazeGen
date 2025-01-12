#include "Window.h"
#include "../App.h"

#include "imgui.h"
#include "imgui_impl_win32.h"

MouseButtonEventArgs::MouseButton DecodeMouseButton(UINT messageID)
{
    MouseButtonEventArgs::MouseButton mouseButton = MouseButtonEventArgs::None;
    switch (messageID)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Left;
    }
    break;
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Right;
    }
    break;
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK:
    {
        mouseButton = MouseButtonEventArgs::Middel;
    }
    break;
    }

    return mouseButton;
}
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
        return true;

    switch (message)
    {
    case WM_PAINT:
    {
        // Delta time will be filled in by the Window.
        UpdateEventArgs updateEventArgs(0.0f, 0.0f);
        GAME_WINDOW->OnUpdate(updateEventArgs);
        RenderEventArgs renderEventArgs(0.0f, 0.0f);
        // Delta time will be filled in by the Window.
        GAME_WINDOW->OnRender(renderEventArgs);
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
    {
        MSG charMsg;
        // Get the Unicode character (UTF-16)
        unsigned int c = 0;
        // For printable characters, the next message will be WM_CHAR.
        // This message contains the character code we need to send the KeyPressed event.
        // Inspired by the SDL 1.2 implementation.
        if (PeekMessage(&charMsg, hwnd, 0, 0, PM_NOREMOVE) && charMsg.message == WM_CHAR)
        {
            GetMessage(&charMsg, hwnd, 0, 0);
            c = static_cast<unsigned int>(charMsg.wParam);
        }
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        KeyCode::Key key = (KeyCode::Key)wParam;
        unsigned int scanCode = (lParam & 0x00FF0000) >> 16;
        KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Pressed, shift, control, alt);
        GAME_WINDOW->OnKeyPressed(keyEventArgs);
    }
    break;
    case WM_SYSKEYUP:
    case WM_KEYUP:
    {
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        KeyCode::Key key = (KeyCode::Key)wParam;
        unsigned int c = 0;
        unsigned int scanCode = (lParam & 0x00FF0000) >> 16;

        // Determine which key was released by converting the key code and the scan code
        // to a printable character (if possible).
        // Inspired by the SDL 1.2 implementation.
        unsigned char keyboardState[256];
        GetKeyboardState(keyboardState);
        wchar_t translatedCharacters[4];
        if (int result = ToUnicodeEx(static_cast<UINT>(wParam), scanCode, keyboardState, translatedCharacters, 4, 0, NULL) > 0)
        {
            c = translatedCharacters[0];
        }

        KeyEventArgs keyEventArgs(key, c, KeyEventArgs::Released, shift, control, alt);
        GAME_WINDOW->OnKeyReleased(keyEventArgs);
    }
    break;
    // The default window procedure will play a system notification sound 
    // when pressing the Alt+Enter keyboard combination if this message is 
    // not handled.
    case WM_SYSCHAR:
        break;
    case WM_MOUSEMOVE:
    {
        bool lButton = (wParam & MK_LBUTTON) != 0;
        bool rButton = (wParam & MK_RBUTTON) != 0;
        bool mButton = (wParam & MK_MBUTTON) != 0;
        bool shift = (wParam & MK_SHIFT) != 0;
        bool control = (wParam & MK_CONTROL) != 0;

        int x = ((int)(short)LOWORD(lParam));
        int y = ((int)(short)HIWORD(lParam));

        MouseMotionEventArgs mouseMotionEventArgs(lButton, mButton, rButton, control, shift, x, y);
        GAME_WINDOW->OnMouseMoved(mouseMotionEventArgs);
    }
    break;
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
        bool lButton = (wParam & MK_LBUTTON) != 0;
        bool rButton = (wParam & MK_RBUTTON) != 0;
        bool mButton = (wParam & MK_MBUTTON) != 0;
        bool shift = (wParam & MK_SHIFT) != 0;
        bool control = (wParam & MK_CONTROL) != 0;

        int x = ((int)(short)LOWORD(lParam));
        int y = ((int)(short)HIWORD(lParam));

        MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Pressed, lButton, mButton, rButton, control, shift, x, y);
        GAME_WINDOW->OnMouseButtonPressed(mouseButtonEventArgs);
    }
    break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        bool lButton = (wParam & MK_LBUTTON) != 0;
        bool rButton = (wParam & MK_RBUTTON) != 0;
        bool mButton = (wParam & MK_MBUTTON) != 0;
        bool shift = (wParam & MK_SHIFT) != 0;
        bool control = (wParam & MK_CONTROL) != 0;

        int x = ((int)(short)LOWORD(lParam));
        int y = ((int)(short)HIWORD(lParam));

        MouseButtonEventArgs mouseButtonEventArgs(DecodeMouseButton(message), MouseButtonEventArgs::Released, lButton, mButton, rButton, control, shift, x, y);
        GAME_WINDOW->OnMouseButtonReleased(mouseButtonEventArgs);
    }
    break;
    case WM_MOUSEWHEEL:
    {
        // The distance the mouse wheel is rotated.
        // A positive value indicates the wheel was rotated to the right.
        // A negative value indicates the wheel was rotated to the left.
        float zDelta = ((int)(short)HIWORD(wParam)) / (float)WHEEL_DELTA;
        short keyStates = (short)LOWORD(wParam);

        bool lButton = (keyStates & MK_LBUTTON) != 0;
        bool rButton = (keyStates & MK_RBUTTON) != 0;
        bool mButton = (keyStates & MK_MBUTTON) != 0;
        bool shift = (keyStates & MK_SHIFT) != 0;
        bool control = (keyStates & MK_CONTROL) != 0;

        int x = ((int)(short)LOWORD(lParam));
        int y = ((int)(short)HIWORD(lParam));

        // Convert the screen coordinates to client coordinates.
        POINT clientToScreenPoint;
        clientToScreenPoint.x = x;
        clientToScreenPoint.y = y;
        ScreenToClient(hwnd, &clientToScreenPoint);

        MouseWheelEventArgs mouseWheelEventArgs(zDelta, lButton, mButton, rButton, control, shift, (int)clientToScreenPoint.x, (int)clientToScreenPoint.y);
        GAME_WINDOW->OnMouseWheel(mouseWheelEventArgs);
    }
    break;
    case WM_SIZE:
    {
        int width = ((int)(short)LOWORD(lParam));
        int height = ((int)(short)HIWORD(lParam));

        ResizeEventArgs resizeEventArgs(width, height);
        GAME_WINDOW->OnResize(resizeEventArgs);
    }
    break;
    case WM_DESTROY:
    {
        // If a window is being destroyed, remove it from the 
        // window maps.
        GAME_WINDOW->Close();

        PostQuitMessage(0);
    }
    break;
    default:
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    

    return 0;
}
static void RegisterWindowClass(HINSTANCE hInst, const wchar_t* windowClassName)
{
	// Register a window class for creating our render window with.
	WNDCLASSEXW windowClass = {};

	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = &WndProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = hInst;
	windowClass.hIcon = ::LoadIcon(hInst, NULL);
	windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	windowClass.lpszMenuName = NULL;
	windowClass.lpszClassName = windowClassName;
	windowClass.hIconSm = ::LoadIcon(hInst, NULL);

	static ATOM atom = ::RegisterClassExW(&windowClass);
	assert(atom > 0);
}


static HWND CreateWindow(const wchar_t* windowClassName, HINSTANCE hInst,
	const wchar_t* windowTitle, uint32_t width, uint32_t height)
{
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	int windowWidth = windowRect.right - windowRect.left;
	int windowHeight = windowRect.bottom - windowRect.top;

	// Center the window within the screen. Clamp to 0, 0 for the top-left corner.
	int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
	int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

	HWND hWnd = ::CreateWindowExW(
		NULL,
		windowClassName,
		windowTitle,
		WS_OVERLAPPEDWINDOW,
		windowX,
		windowY,
		windowWidth,
		windowHeight,
		NULL,
		NULL,
		hInst,
		nullptr
	);

	assert(hWnd && "Failed to create window");

	return hWnd;
}

Window::Window(HINSTANCE hInstance) {

	RegisterWindowClass(hInstance, Globals::STARTUP_VALS.window_className);
	m_handle = CreateWindow(Globals::STARTUP_VALS.window_className, hInstance, L"Learning DirectX 12",
		Globals::STARTUP_VALS.window_width, Globals::STARTUP_VALS.window_height);

	// Initialize the global window rect variable.
	::GetWindowRect(m_handle, &m_rect);
}

void Window::ToggleFullscreen()
{
	m_fullscreen = !m_fullscreen;
	if (m_fullscreen) // Switching to fullscreen.
	{
		// Store the current window dimensions so they can be restored 
		// when switching out of fullscreen state.
		::GetWindowRect(m_handle, &m_rect);
		// Set the window style to a borderless window so the client area fills
// the entire screen.
		UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

		::SetWindowLongW(m_handle, GWL_STYLE, windowStyle);
		// Query the name of the nearest display device for the window.
		// This is required to set the fullscreen dimensions of the window
		// when using a multi-monitor setup.
		HMONITOR hMonitor = ::MonitorFromWindow(m_handle, MONITOR_DEFAULTTONEAREST);
		MONITORINFOEX monitorInfo = {};
		monitorInfo.cbSize = sizeof(MONITORINFOEX);
		::GetMonitorInfo(hMonitor, &monitorInfo);
		::SetWindowPos(m_handle, HWND_TOP,
			monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.top,
			monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
			monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_handle, SW_MAXIMIZE);
	}
	else
	{
		// Restore all the window decorators.
		::SetWindowLong(m_handle, GWL_STYLE, WS_OVERLAPPEDWINDOW);

		::SetWindowPos(m_handle, HWND_NOTOPMOST,
			m_rect.left,
			m_rect.top,
			m_rect.right - m_rect.left,
			m_rect.bottom - m_rect.top,
			SWP_FRAMECHANGED | SWP_NOACTIVATE);

		::ShowWindow(m_handle, SW_NORMAL);
	}
}
void Window::RegisterCallbacks(std::shared_ptr<App> app) {
    m_app = app;
}
void Window::Show() {
	::ShowWindow(m_handle, SW_SHOW);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
		}
	}

}

void Window::Close() {
    if (m_app)
        m_app->OnWindowDestroy();
	//DestroyWindow(m_handle);
}

HWND Window::GetHandle() const {
	return m_handle;
}

RECT Window::GetRect() const {
	return m_rect;
}
int Window::GetWidth() const {
	return m_width;
}
int Window::GetHeight() const {
	return m_height;
}

void Window::OnUpdate(UpdateEventArgs& e) {
	if (m_app)
		m_app->OnUpdate(e);
}
void Window::OnRender(RenderEventArgs& e) {
	if (m_app)
		m_app->OnRender(e);
}

void Window::OnKeyPressed(KeyEventArgs& e) {
    if (m_app)
        m_app->OnKeyPressed(e);
}
void Window::OnKeyReleased(KeyEventArgs& e) {
    if (m_app)
        m_app->OnKeyReleased(e);
}

void Window::OnMouseMoved(MouseMotionEventArgs& e) {
    if (m_app)
        m_app->OnMouseMoved(e);
}
void Window::OnMouseButtonPressed(MouseButtonEventArgs& e) {
    if (m_app)
        m_app->OnMouseButtonPressed(e);
}
void Window::OnMouseButtonReleased(MouseButtonEventArgs& e) {
    if (m_app)
        m_app->OnMouseButtonReleased(e);
}
void Window::OnMouseWheel(MouseWheelEventArgs& e) {
    if (m_app)
        m_app->OnMouseWheel(e);
}

void Window::OnResize(ResizeEventArgs& e) {
    RECT clientRect = {};
    ::GetClientRect(m_handle, &clientRect);

    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    if (m_width != width || m_height != height)
    {

        // Don't allow 0 size swap chain back buffers.
        m_width = std::max(1, width);
        m_height = std::max(1, height);
    }
    if (m_app)
        m_app->OnResize(e);
}