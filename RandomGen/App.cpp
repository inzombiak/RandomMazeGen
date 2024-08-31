#include "App.h"

#include "Rendering/Window.h"
#include "Rendering/Renderer_D12.h"


App::App(const std::wstring& name, int width, int height, bool vSync, HINSTANCE hInstance)
    : m_name(name)
    , m_width(width)
    , m_height(height)
    , m_vSync(vSync)
    , m_hInstance(hInstance)
{
}

App::~App()
{
    Destroy();
}

std::shared_ptr<Window> GAME_WINDOW;
std::shared_ptr<Renderer_D12> RENDERER;
Globals::InputState Globals::INPUT_STATE;
bool App::Initialize()
{
    // Check for DirectX Math library support.
    if (!DirectX::XMVerifyCPUSupport())
    {
        MessageBoxA(NULL, "Failed to verify DirectX Math library support.", "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    GAME_WINDOW = std::make_shared<Window>(m_hInstance);
    GAME_WINDOW->RegisterCallbacks(shared_from_this());
    RENDERER = std::make_shared<Renderer_D12>();
    GAME_WINDOW->Show();

    return true;
}

bool App::LoadContent() {
    return true;
}

void App::UnloadContent() {

}

void App::Destroy()
{

}

void App::OnUpdate(UpdateEventArgs& e)
{

}

void App::OnRender(RenderEventArgs& e)
{
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->Render();
}

void App::OnKeyPressed(KeyEventArgs& e)
{
    // By default, do nothing.
}

void App::OnKeyReleased(KeyEventArgs& e)
{
    // By default, do nothing.
}

void App::OnMouseMoved(class MouseMotionEventArgs& e)
{
    Globals::INPUT_STATE.mousePos = sf::Vector2i(e.X, e.Y);

    if (Globals::INPUT_STATE.mouseBtnState & MK_LBUTTON) {
        Globals::INPUT_STATE.lastMouseDownPos = Globals::INPUT_STATE.mousePos;
    }
}

void App::OnMouseButtonPressed(MouseButtonEventArgs& e){
    if(e.LeftButton)
        Globals::INPUT_STATE.mouseBtnState |= MK_LBUTTON;
    if (e.MiddleButton)
        Globals::INPUT_STATE.mouseBtnState |= MK_MBUTTON;
    if (e.RightButton)
        Globals::INPUT_STATE.mouseBtnState |= MK_RBUTTON;
}

void App::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    if (e.LeftButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_LBUTTON;
    if (e.MiddleButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_MBUTTON;
    if (e.RightButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_RBUTTON;
}

void App::OnMouseWheel(MouseWheelEventArgs& e)
{
    // By default, do nothing.
}

void App::OnResize(ResizeEventArgs& e)
{
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->ResizeTargets();
    m_width = e.Width;
    m_height = e.Height;
}

void App::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->Shutdown();
}