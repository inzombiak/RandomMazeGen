#include "App.h"

#include "Rendering/Window.h"
#include "Rendering/Renderer_D12.h"
#include "GridManager.h"

using namespace DirectX;
static VertexInput BOX_VERTICES[8] = {
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) }, // 0
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, // 1
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, // 2
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, // 3
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, // 4
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, // 5
    { XMFLOAT3(1.0f,  1.0f,  1.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, // 6
    { XMFLOAT3(1.0f, -1.0f,  1.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }  // 7
};

static WORD BOX_INDICES[36] =
{
    0, 1, 2, 0, 2, 3,
    4, 6, 5, 4, 7, 6,
    4, 5, 1, 4, 1, 0,
    3, 2, 6, 3, 6, 7,
    1, 5, 6, 1, 6, 2,
    4, 0, 3, 4, 3, 7
};


App::App(const std::wstring& name, int width, int height, bool vSync, HINSTANCE hInstance)
    : m_name(name)
    , m_width(width)
    , m_height(height)
    , m_vSync(vSync)
    , m_hInstance(hInstance)
    , m_contentLoaded(false)
    , m_gridManager(std::make_shared<GridManager>())
{
    m_cameraPos    = XMVectorSet(0, 0, -10, 1);
    m_camAngles[0] = 0;
    m_camAngles[1] = 0;
    m_camAngles[2] = 0;
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
    LoadContent();
    GAME_WINDOW->Show();

    return true;
}

bool App::LoadContent() {

    if (!RENDERER || !RENDERER->IsInitialized())
        return false;

    m_gridManager->GenerateMap(m_width, m_height, 8, 8 );

    RENDERER->PopulateVertexBuffer(BOX_VERTICES, _countof(BOX_VERTICES));
    RENDERER->PopulateIndexBuffer(BOX_INDICES, _countof(BOX_INDICES));
    RENDERER->CreateSRVForBoxes(m_gridManager->GetTiles(), 0);
    RENDERER->BuildPipelineState(L"C:/Projects/RandomMazeGen/x64/Debug/vertex_basic.cso", L"C:/Projects/RandomMazeGen/x64/Debug/pixel_basic.cso");
    RENDERER->ResizeDepthBuffer(m_width, m_height);

    m_contentLoaded = true;
    return true;
}

void App::UnloadContent() {

}

void App::Destroy()
{

}

void App::OnUpdate(UpdateEventArgs& e)
{
    m_updateClock.Tick();
    static uint64_t frameCount = 0;
    static double totalTime = 0.0;

    totalTime += m_updateClock.GetDeltaSeconds();
    frameCount++;

    if (totalTime > 1.0)
    {
        double fps = frameCount / totalTime;

        char buffer[512];
        sprintf_s(buffer, "FPS: %f\n", fps);
        OutputDebugStringA(buffer);

        frameCount = 0;
        totalTime = 0.0;
    }

    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(m_camAngles[0], m_camAngles[1], m_camAngles[2]);
    XMMATRIX orientation = XMMatrixRotationQuaternion(quaternion);

    auto camRight = orientation.r[0];
    auto camUp = orientation.r[1];
    auto camFwd = orientation.r[2];
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::W]){
        m_cameraPos += camFwd * Globals::CAM_PAN_SPEED;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::S]) {
        m_cameraPos -= camFwd * Globals::CAM_PAN_SPEED;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::D]) {
        m_cameraPos += camRight * Globals::CAM_PAN_SPEED;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::A]) {
        m_cameraPos -= camRight * Globals::CAM_PAN_SPEED;
    }
    //m_updateClock.GetTotalSeconds();
    RENDERER->UpdateMVP(m_fov, m_cameraPos, camFwd, camRight, camUp);
}

void App::OnRender(RenderEventArgs& e)
{
    m_renderClock.Tick();
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->Render();
}

void App::OnKeyPressed(KeyEventArgs& e)
{
    Globals::INPUT_STATE.keyStates[e.Key] = true;
}

void App::OnKeyReleased(KeyEventArgs& e)
{
    Globals::INPUT_STATE.keyStates[e.Key] = false;
}

void App::OnMouseMoved(class MouseMotionEventArgs& e)
{
    Globals::INPUT_STATE.mousePos = sf::Vector2i(e.X, e.Y);
    if (Globals::INPUT_STATE.mouseBtnState & MK_RBUTTON) {
        int dx = Globals::INPUT_STATE.mousePos.x - Globals::INPUT_STATE.lastMouseDownPos.x;
        int dy = Globals::INPUT_STATE.mousePos.y - Globals::INPUT_STATE.lastMouseDownPos.y;

        m_camAngles[1] += dx * Globals::CAM_ROT_SPEED;
        m_camAngles[0] += dy * Globals::CAM_ROT_SPEED;
    }

    Globals::INPUT_STATE.lastMouseDownPos = Globals::INPUT_STATE.mousePos;
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
    if (!e.LeftButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_LBUTTON;
    if (!e.MiddleButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_MBUTTON;
    if (!e.RightButton)
        Globals::INPUT_STATE.mouseBtnState ^= MK_RBUTTON;
}
#include <algorithm>
void App::OnMouseWheel(MouseWheelEventArgs& e)
{
    m_fov -= e.WheelDelta;
    m_fov = std::clamp(m_fov, 12.0f, 90.0f);

    char buffer[256];
    sprintf_s(buffer, "FoV: %f\n", m_fov);
    OutputDebugStringA(buffer);
}

void App::OnResize(ResizeEventArgs& e)
{
    if (m_width == e.Width && m_height == e.Height)
        return;
    m_width = e.Width;
    m_height = e.Height;
    if (RENDERER && RENDERER->IsInitialized()) {
        RENDERER->ResizeTargets();
        RENDERER->ResizeDepthBuffer(m_width, m_height);
    }
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