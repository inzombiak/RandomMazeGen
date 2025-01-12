#include "App.h"

#include "Rendering/Window.h"
#include "Rendering/Renderer_D12.h"
#include "GridManager.h"

#include <iostream>

using namespace DirectX;
static VertexInput BOX_VERTICES[24] = {
    //Size Za
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    { XMFLOAT3(1.0f,  1.0f, -1.0f),  XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
    { XMFLOAT3(1.0f, -1.0f, -1.0f),  XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

    //Size Zb
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) }, 
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) }, 
    { XMFLOAT3(1.0f,  1.0f,  1.0f),  XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) }, 
    { XMFLOAT3(1.0f, -1.0f,  1.0f),  XMFLOAT3(1.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },  

    //Side Xa
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    { XMFLOAT3(-1.0f,  1.0f, 1.0f),  XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
    { XMFLOAT3(-1.0f, -1.0f, 1.0f),  XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

    //Side Xb
    { XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
    { XMFLOAT3(1.0f,  1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
    { XMFLOAT3(1.0f,  1.0f, 1.0f),  XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
    { XMFLOAT3(1.0f, -1.0f, 1.0f),  XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f) },

    //Top
    { XMFLOAT3(-1.0f,  1.0f, -1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) }, 
    { XMFLOAT3(1.0f,  1.0f, -1.0f),  XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) }, 
    { XMFLOAT3(-1.0f,  1.0f,  1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) }, 
    { XMFLOAT3(1.0f,  1.0f,  1.0f),  XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) }, 

    //Bot
    { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
    { XMFLOAT3(1.0f,  -1.0f, -1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
    { XMFLOAT3(-1.0f, -1.0f,  1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
    { XMFLOAT3(1.0f,  -1.0f,  1.0f), XMFLOAT3(0.6f, 1.0f, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
};

static WORD BOX_INDICES[36] =
{   
    //Za
    0, 1, 2, 0, 2, 3,
    //Zb
    4, 6, 5, 4, 7, 6,

    //Xa
    8, 10, 9, 8, 11, 10,
    //Xb
    12, 13, 14, 12, 14, 15,
    
    //Top
    16, 18, 19, 16, 19, 17,
    //Bot
    20, 23, 22, 20, 21, 22
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
    m_cameraPos    = XMVectorSet(-17, 26.7f, 16, 1);
    m_sunPos       = XMVectorSet(42, 38.7f, 29, 1);
    m_camAngles[0] = 0.66f;
    m_camAngles[1] = 1.57f;
    m_camAngles[2] = 0;
}

App::~App()
{
}

std::shared_ptr<Window> GAME_WINDOW;
std::shared_ptr<Renderer_D12> RENDERER;
Globals::InputState Globals::INPUT_STATE;

//@ZGTODO streamline
inline bool GUIActive() {
    if (RENDERER && RENDERER->GUIInitialized()) {
        auto& io = ImGui::GetIO();
        return io.WantCaptureMouse || io.WantCaptureKeyboard;
    }
    return false;
}
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
    RENDERER->PostInit();
    LoadContent();
    GAME_WINDOW->Show();

    return true;
}

bool App::LoadContent() {

    if (!RENDERER || !RENDERER->IsInitialized())
        return false;

    RENDERER->PopulateVertexBuffer(BOX_VERTICES, _countof(BOX_VERTICES));
    RENDERER->PopulateIndexBuffer(BOX_INDICES, _countof(BOX_INDICES));
    //m_gridManager->RandomizeMap();
    m_gridManager->GenerateMap(m_width, m_height, 16, 16);
    RENDERER->BuildPipelineState(L"vertex_basic.cso", L"pixel_basic.cso");
    RENDERER->BuildShadowPipelineState(L"vertex_shadow.cso", L"pixel_shadow.cso");
    RENDERER->LoadTextures();
    RENDERER->ResizeDepthBuffer(m_width, m_height);

    m_contentLoaded = true;
    return true;
}

void App::UnloadContent() {

}

void App::Destroy()
{
    if (RENDERER && RENDERER->IsInitialized())
        RENDERER->Shutdown();
}

void App::OnUpdate(UpdateEventArgs& e)
{

    m_updateClock.Tick();
    static uint64_t frameCount = 0;
    static double totalTime = 0.0;
    double dt = m_updateClock.GetDeltaSeconds();
    totalTime += dt;
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
    RENDERER->CreateSRVForBoxes(m_gridManager->GetTiles(), 0);

    if (RENDERER && RENDERER->GUIInitialized()) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Controls");
        if (ImGui::CollapsingHeader("Map Generation"))
        {
            ImGui::Combo("Maze Algorithm", &m_mazeAlgorithm, GameDefs::MazeAlgorithmLabels, 2);
            m_gridManager->SetMazeAlgorithm(GameDefs::MazeAlgorithm(m_mazeAlgorithm));

            ImGui::Combo("Generation Type", &m_generationType, GameDefs::GenerateTypeLabels, 2);
            m_gridManager->SetMazeGenerateType(GameDefs::GenerateType(m_generationType));

            ImGui::PushItemWidth(100);
            ImGui::InputInt("Rows", &m_rows);
            m_rows = std::clamp(m_rows, 1, 100);
            ImGui::SameLine();
            ImGui::InputInt("Columns", &m_columns);
            m_columns = std::clamp(m_columns, 1, 100);
            ImGui::SameLine();
            bool regen = ImGui::Button("Regenerate");
            if (regen)
                m_gridManager->GenerateMap(m_width, m_height, 16, 16);

        }
        ImGui::End();
    }


    if (GUIActive()) 
        return;

    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::W]) {
        m_cameraPos += camFwd * Globals::CAM_PAN_SPEED * dt;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::S]) {
        m_cameraPos -= camFwd * Globals::CAM_PAN_SPEED * dt;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::D]) {
        m_cameraPos += camRight * Globals::CAM_PAN_SPEED * dt;
    }
    if (Globals::INPUT_STATE.keyStates[KeyCode::Key::A]) {
        m_cameraPos -= camRight * Globals::CAM_PAN_SPEED * dt;
    }


    //m_updateClock.GetTotalSeconds();
    RENDERER->UpdateMVP(m_fov, m_cameraPos, camFwd, camRight, camUp);
}

void App::OnRender(RenderEventArgs& e)
{
    m_renderClock.Tick();
    if (RENDERER && RENDERER->IsInitialized()) {
        RENDERER->Shadowmap(m_sunPos);
        RENDERER->Render();
    }
}

void App::OnKeyPressed(KeyEventArgs& e)
{
}

void App::OnKeyReleased(KeyEventArgs& e)
{
    Globals::INPUT_STATE.keyStates[e.Key] = false;
}

void App::OnMouseMoved(class MouseMotionEventArgs& e)
{
    if (GUIActive())
        return;

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
        Globals::INPUT_STATE.mouseBtnState &= ~MK_LBUTTON;
    if (!e.MiddleButton)
        Globals::INPUT_STATE.mouseBtnState &= ~MK_MBUTTON;
    if (!e.RightButton)
        Globals::INPUT_STATE.mouseBtnState &= ~MK_RBUTTON;
}
#include <algorithm>
void App::OnMouseWheel(MouseWheelEventArgs& e)
{
    if (GUIActive())
        return;

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
}