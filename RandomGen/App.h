#pragma once

#include "Events.h"
#include "HighResolutionClock.h"
#include <DirectXMath.h>

#include <memory>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

class GridManager;
class App : public std::enable_shared_from_this<App>
{
public:
    /**
     * Create the DirectX demo using the specified window dimensions.
     */
    App(const std::wstring& name, int width, int height, bool vSync, HINSTANCE hInstance);
    virtual ~App();

    int GetClientWidth() const
    {
        return m_width;
    }

    int GetClientHeight() const
    {
        return m_height;
    }

    /**
     *  Initialize the DirectX Runtime.
     */
    bool Initialize();

    /**
     *  Load content required for the demo.
     */
    bool LoadContent();

    /**
     *  Unload demo specific content that was loaded in LoadContent.
     */
    void UnloadContent();

    /**
     * Destroy any resource that are used by the game.
     */
    void Destroy();

protected:
    friend class Window;

    /**
     *  Update the game logic.
     */
    void OnUpdate(UpdateEventArgs& e);

    /**
     *  Render stuff.
     */
    void OnRender(RenderEventArgs& e);

    /**
     * Invoked by the registered window when a key is pressed
     * while the window has focus.
     */
    void OnKeyPressed(KeyEventArgs& e);

    /**
     * Invoked when a key on the keyboard is released.
     */
    void OnKeyReleased(KeyEventArgs& e);

    /**
     * Invoked when the mouse is moved over the registered window.
     */
    void OnMouseMoved(MouseMotionEventArgs& e);

    /**
     * Invoked when a mouse button is pressed over the registered window.
     */
    void OnMouseButtonPressed(MouseButtonEventArgs& e);

    /**
     * Invoked when a mouse button is released over the registered window.
     */
    void OnMouseButtonReleased(MouseButtonEventArgs& e);

    /**
     * Invoked when the mouse wheel is scrolled while the registered window has focus.
     */
    void OnMouseWheel(MouseWheelEventArgs& e);

    /**
     * Invoked when the attached window is resized.
     */
    void OnResize(ResizeEventArgs& e);

    /**
     * Invoked when the registered window instance is destroyed.
     */
    void OnWindowDestroy();
private:
    std::shared_ptr<GridManager> m_gridManager;

    HINSTANCE m_hInstance;

    HighResolutionClock m_updateClock;
    HighResolutionClock m_renderClock;

    int m_rows = 16;
    int m_columns = 16;
    int m_generationType = 0;
    int m_mazeAlgorithm = 0;

    float m_fov = 45.0;
    DirectX::XMVECTOR m_cameraPos;
    float m_camAngles[3];
    DirectX::XMVECTOR m_sunPos;

    std::wstring m_name;
    int m_width;
    int m_height;
    bool m_vSync;
    bool m_contentLoaded;
};