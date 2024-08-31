#include "MazeGenApp.h"

#include "Rendering/Window.h"
#include "Rendering/Renderer_D12.h"


MazeGenApp::MazeGenApp(const std::wstring& name, int width, int height, bool vSync, HINSTANCE hInstance) : App(name, width, height, vSync, hInstance)
{
}

MazeGenApp::~MazeGenApp()
{
}

bool MazeGenApp::Initialize()
{
    return App::Initialize();
}

void MazeGenApp::Destroy()
{
    App::Destroy();
}

bool MazeGenApp::LoadContent() {
    return true;
}

void MazeGenApp::UnloadContent() {

}

void MazeGenApp::OnUpdate(UpdateEventArgs& e)
{

}

void MazeGenApp::OnRender(RenderEventArgs& e)
{
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->Render();
}

void MazeGenApp::OnKeyPressed(KeyEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnKeyReleased(KeyEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnMouseMoved(class MouseMotionEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnMouseButtonPressed(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnMouseButtonReleased(MouseButtonEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnMouseWheel(MouseWheelEventArgs& e)
{
    // By default, do nothing.
}

void MazeGenApp::OnResize(ResizeEventArgs& e)
{
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->ResizeTargets();
    App::OnResize(e);
}

void MazeGenApp::OnWindowDestroy()
{
    // If the Window which we are registered to is 
    // destroyed, then any resources which are associated 
    // to the window must be released.
    UnloadContent();
    App::OnWindowDestroy();
}