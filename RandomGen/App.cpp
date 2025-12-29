#include "App.h"

#include "Rendering/Window.h"
#include "Rendering/Renderer_D12.h"
#include "MazeGenDefs.h"

#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


static VertexInput BOX_VERTICES[24] = {
    //Size Za
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f,  1.0f, -1.0f),  glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f, -1.0f, -1.0f),  glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

    //Size Zb
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f,  1.0f,  1.0f),  glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f, -1.0f,  1.0f),  glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 0.0f) },

    //Side Xa
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3(-1.0f,  1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) },
    { glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

    //Side Xb
    { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f) },
    { glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f,  1.0f, 1.0f),  glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.0f, 0.0f) },
    { glm::vec3(1.0f, -1.0f, 1.0f),  glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f) },

    //Top
    { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { glm::vec3(1.0f,  1.0f, -1.0f),  glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f) },
    { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f) },
    { glm::vec3(1.0f,  1.0f,  1.0f),  glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) },

    //Bot
    { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
    { glm::vec3(1.0f,  -1.0f, -1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 1.0f) },
    { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 1.0f) },
    { glm::vec3(1.0f,  -1.0f,  1.0f), glm::vec3(0.6f, 1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) },
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
{
    m_cameraPos    = glm::vec3(-17, 26.7f, 16);
    m_sunPos       = glm::vec4(0, 38.7f, 29, 1);
    m_camAngles    = glm::vec3(0.66f, 1.57f, 0);
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

    m_tileProperties.resize(m_rows);
    for (int i = 0; i < m_rows; ++i) {
        m_tileProperties[i].resize(m_columns);
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
    GenerateMap(m_width, m_height, m_rows, m_columns);
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

// Function to convert from XYZ position to an angle in the ZY plane
float GetAngleOnZYPlane(const glm::vec4& position) {
    // Extract the Z and Y components of the position vector
    return atan2f(position.z, position.y); // atan2(z, y) gives the angle in radians
}

// Function to convert from an angle in the ZY plane back to an XYZ position
glm::vec4 GetPositionFromAngle(float angle, float radius = 1.0f) {
    // Compute the Y and Z components based on the angle
    float y = radius * cosf(angle); // y = r * cos(angle)
    float z = radius * sinf(angle); // z = r * sin(angle)

    // Return the new position as a vec4 (x = 0, y, z, w = 1)
    return glm::vec4(0.0f, y, z, 1.0f);
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

    glm::quat quaternion = glm::quat(m_camAngles);
    glm::mat4 orientation = glm::mat4_cast(quaternion);

    glm::vec3 camRight = glm::vec3(orientation[0]);
    glm::vec3 camUp = glm::vec3(orientation[1]);
    glm::vec3 camFwd = glm::vec3(orientation[2]);

    // OPTIMIZATION: Only query and upload tiles if maze has changed
    // Both Full and Step modes now use the dirty flag system
    // Step mode adds frame rate throttling to limit updates to ~60fps for smooth visualization
    static auto lastStepQuery = std::chrono::high_resolution_clock::now();
    const int STEP_QUERY_INTERVAL_MS = 16; // ~60fps max for step visualization

    bool shouldQuery = false;

    // Check if maze is dirty (tiles have changed)
    if (IsMazeDirty()) {
        // In Step mode, also check frame rate throttling to avoid excessive updates
        if (m_generationType == MazeDefs::Step) {
            auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStepQuery).count();
            if (elapsed >= STEP_QUERY_INTERVAL_MS) {
                shouldQuery = true;
                lastStepQuery = now;
            }
        } else {
            // Full mode: query immediately when dirty
            shouldQuery = true;
        }
    }

    if (shouldQuery) {
        // Use batch API for efficiency (single DLL call instead of rows*columns calls)
        // Note: m_tileProperties is a 2D vector, so we need a temporary contiguous buffer
        std::vector<MazeDefs::TileProperties> tempBuffer(m_rows * m_columns);
        GetAllTileProperties(tempBuffer.data(), m_rows * m_columns);

        // Copy from temp buffer to 2D structure
        for (int i = 0; i < m_rows; ++i) {
            for (int j = 0; j < m_columns; ++j) {
                m_tileProperties[i][j] = tempBuffer[i * m_columns + j];
            }
        }

        // Upload to GPU
        RENDERER->CreateSRVForBoxes(m_tileProperties, m_rows, m_columns, 0);

        // Clear dirty flag after sync for both modes
        ClearMazeDirtyFlag();
    }

    if (RENDERER && RENDERER->GUIInitialized()) {
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Controls");
        if (ImGui::CollapsingHeader("Map Generation"))
        {
            ImGui::Combo("Maze Algorithm", &m_mazeAlgorithm, MazeDefs::MazeAlgorithmLabels, 2);
            SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm(m_mazeAlgorithm));

            ImGui::Combo("Generation Type", &m_generationType, MazeDefs::GenerateTypeLabels, 2);
            SetMazeGenerationType(MazeDefs::GenerateType(m_generationType));

            ImGui::PushItemWidth(100);
            ImGui::InputInt("Rows", &m_rows);
            m_rows = std::clamp(m_rows, 10, 100);
            ImGui::SameLine();
            ImGui::InputInt("Columns", &m_columns);
            m_columns = std::clamp(m_columns, 10, 100);
            ImGui::SameLine();
            bool regen = ImGui::Button("Regenerate");
            if (regen) {
                m_tileProperties.resize(m_rows);
                for (int i = 0; i < m_rows; ++i) {
                    m_tileProperties[i].resize(m_columns);
                }
                GenerateMap(m_width, m_height, m_rows, m_columns);
            }
        }

        if (ImGui::CollapsingHeader("Atmosphere")) {

            float radius = glm::length(glm::vec3(m_sunPos));

            // Convert the current sun position to an angle in the ZY plane
            float angle = GetAngleOnZYPlane(m_sunPos);
            float timeOfDay = ((angle / (float)M_PI) + 1) * 12.f;
            // Use ImGui slider to modify the angle (range from -PI to +PI)
            ImGui::SliderFloat("Sun Angle 6AM-6PM", &timeOfDay, 6, 18);
            //timeOfDay += 5 * dt;
            
            angle = (timeOfDay - 12) * (float)M_PI / 12.f;


            // Convert the angle back to an XYZ position
            m_sunPos = GetPositionFromAngle(angle, radius);
        }

        ImGui::End();
    }


    if (!GUIActive()) {

        if (Globals::INPUT_STATE.keyStates[KeyCode::Key::W]) {
            m_cameraPos += camFwd * Globals::CAM_PAN_SPEED * (float)dt;
        }
        if (Globals::INPUT_STATE.keyStates[KeyCode::Key::S]) {
            m_cameraPos -= camFwd * Globals::CAM_PAN_SPEED * (float)dt;
        }
        if (Globals::INPUT_STATE.keyStates[KeyCode::Key::D]) {
            m_cameraPos += camRight * Globals::CAM_PAN_SPEED * (float)dt;
        }
        if (Globals::INPUT_STATE.keyStates[KeyCode::Key::A]) {
            m_cameraPos -= camRight * Globals::CAM_PAN_SPEED * (float)dt;
        }
    }

    //m_updateClock.GetTotalSeconds();
    RENDERER->UpdateMVP(m_fov, m_cameraPos, camFwd, camRight, camUp, m_sunPos);
}

void App::OnRender(RenderEventArgs& e)
{
    m_renderClock.Tick();
    if (RENDERER && RENDERER->IsInitialized()) {
        RENDERER->Render();
    }
}

void App::OnKeyPressed(KeyEventArgs& e)
{
    Globals::INPUT_STATE.keyStates[e.Key] = true;
    auto io = ImGui::GetIO();
    io.AddInputCharacter(e.Char);
}

void App::OnKeyReleased(KeyEventArgs& e)
{
    Globals::INPUT_STATE.keyStates[e.Key] = false;
}

void App::OnMouseMoved(class MouseMotionEventArgs& e)
{
    if (GUIActive())
        return;

    Globals::INPUT_STATE.mousePos = std::pair<int, int>(e.X, e.Y);
    if (Globals::INPUT_STATE.mouseBtnState & MK_RBUTTON) {
        int dx = Globals::INPUT_STATE.mousePos.first - Globals::INPUT_STATE.lastMouseDownPos.first;
        int dy = Globals::INPUT_STATE.mousePos.second - Globals::INPUT_STATE.lastMouseDownPos.second;

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