# RandomMazeGen — Project Overview and Setup

This document describes the structure, build and runtime setup for the RandomMazeGen repository. At a high level the project is split into two parts:

- MazeGenerator: a C++ library (built as a DLL) that implements procedural dungeon/maze generation algorithms and utilities (room placement, connectors, dead-end removal, tile/grid manager, etc.).
- RandomGen: the DirectX 12-based rendering frontend and application that consumes MazeGenerator to visualize and interact with generated maps.

This file collects the important build/run details, public API surface, the runtime flow, and pointers to the main source locations so you can quickly understand and extend the project.

## Quick start (build & run)

1. Open the solution `RandomGen.sln` in Visual Studio (x64).
2. Select the `RandomGen` project as the startup project and pick the configuration (Debug/Release) and platform `x64`.
3. Build the solution. The build will produce the `RandomGen` executable and the `MazeGenerator` DLL (because the code uses `__declspec(dllexport)` / `dllimport`).
4. Run the `RandomGen` application. The UI exposes ImGui controls to trigger regeneration, change algorithm, toggle step/full generation, and more.

Notes:
- The project is Windows-only and uses DirectX 12 — you need the Windows SDK and a DirectX 12 capable GPU and drivers.
- External helper libraries used by the project are present under `RandomGen/Extern/` (for example `DDSTextureLoader` and `DirectXTex`) and in `imgui/`.
- The repository includes prebuilt textures (`Resources/*.dds`) used by the renderer.

## Public API (MazeGenerator DLL)

The MazeGenerator library exposes a small C-style API for consumers in `MazeGenerator/include/MazeGenDefs.h`:

### Core Generation API
- `void GenerateMap(unsigned int width, unsigned int height, unsigned int rows, unsigned int columns);` — triggers maze generation
- `void SetMazeGenerationType(MazeDefs::GenerateType type);` — sets `Step` or `Full` generation mode
- `void SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm algo);` — chooses between `RecursiveBacktracker` or `EllersAlgorithm`

### Tile Query API (Legacy - Single Tile)
- `const MazeDefs::TileProperties GetTilePropertiesAtIndices(unsigned int i, unsigned int j);` — read individual tile properties

### Optimized Batch API (Recommended)
- `void GetAllTileProperties(MazeDefs::TileProperties* buffer, unsigned int bufferSize);` — batch query all tiles at once (single DLL call)
- `bool IsMazeDirty();` — check if maze has changed since last query
- `void ClearMazeDirtyFlag();` — mark that tiles have been synced

**Performance Note:** The batch API (`GetAllTileProperties`) + dirty flag system reduces DLL boundary crossings from 10,000 calls per frame (for 100×100 maze) to just 1 call when needed. In Step mode with the optimized dirty flag system, queries only occur when tiles actually change, achieving 99%+ reduction in unnecessary work.

These are designed for a C-compatible interface so the renderer can query tile types/directions from the DLL.

Important constants/enums are in `MazeGenDefs.h` (GenerateType, MazeAlgorithm, TileType, PassageDirection) and helper arrays like `DIRECTIONS` and `DIRECTION_CHANGES` used by the algorithms.

If you need to integrate the generator into another project, link against the `MazeGenerator` library and include `MazeGenDefs.h`.

## Architecture and main components

High-level flow:

1. RandomGen (App) sets up window, DX12 renderer and UI.
2. App calls into GridManager to request a new map (rows x columns). GridManager manages a `TileHolder` 2D structure representing tiles.
3. GridManager uses `RoomGenerator` to place rooms, then runs a maze generation algorithm (one of `MARecursiveBacktracker` or `MAEllers`) to carve passages.
4. After generation the `MazeConnector` links rooms/regions and `DeadEndRemover` prunes dead ends. GridManager exposes read-only tile data for the renderer.
5. RandomGen's renderer builds SRVs based on tile properties and draws boxes/tiles representing the maze. ImGui allows switching algorithms and parameters at runtime.

Key source files and responsibilities (quick map):

- MazeGenerator/
  - `include/MazeGenDefs.h` — public API, enums, exported functions
  - `src/GridManager.*` — central orchestrator: room generation, maze generation, connecting map, dead-end removal, threading and synchronization for step-mode
  - `src/MARecursiveBacktracker.*` — recursive backtracker algorithm (supports Step and Full modes)
  - `src/MAEllers.*` — Eller's algorithm implementation (supports Step and Full)
  - `src/MazeConnector.*` — logic for connecting rooms to maze passages
  - `src/RoomGenerator.*` — room placement
  - `src/DeadEndRemover.*` — optional post-processing to remove dead ends
  - `src/Tile.*` — Tile data structure and helpers

- RandomGen/
  - `main.cpp` — WinMain / command-line parsing and App startup
  - `App.cpp` / `App.h` — application life-cycle, input handling, UI (ImGui) and update/render loop. Calls into renderer and generator APIs.
  - `Rendering/*` — DX12 renderer implementation (Renderer_D12, Window, shader loading, descriptor allocators, etc.). This code handles GPU resources, SRV creation, and issuing draw calls.
  - `Resources/` — texture assets and compiled shaders (.cso) referenced by `Renderer_D12`.

Threading and modes:
- Generation supports two modes: `Full` (generate to completion on the calling thread) and `Step` (incremental/visualization mode). In `Step` mode generation uses threads and sleeps so the renderer can visualize intermediate steps.
- GridManager uses std::thread for certain phases (generator, connector, dead-end remover) and coordinates via condition variables to pause/resume between phases when visualizing.

Seed and randomness:
- Seeding is done via std::mt19937 with seeds generated from the system clock by default. The seed is stored and used across generators for deterministic reproducible runs if you pass a fixed seed.

Controls and UI
- The README lists quick keyboard controls used in the app (R - new map, G - toggle algorithm, T - toggle visualization). App's ImGui panel exposes row/column counts, algorithm selection, GenerateType selection (Step/Full), and a Regenerate button.
- Command-line flags are parsed in `main.cpp` (examples: `-w`/`--width`, `-h`/`--height`, `-warp`) — see `ParseCommandLineArguments` in `main.cpp`.

Build / platform details and gotchas

- Target platform: Windows x64. Use Visual Studio (the solution and .vcxproj files are present).
- Ensure you build for x64. The renderer and some compiled shader/code expect 64-bit.
- Required runtime: DirectX 12 capable GPU and drivers. The project uses DirectXMath (DirectX::XM*), DirectX 12 runtime, and helper libs (DirectXTex, DDS loader). Those helper libs are already included under `RandomGen/Extern/`.
- DLL vs static linking: `MazeGenDefs.h` uses `MAZEGENLIB_API` to export/import symbols from a DLL. If you change the project configuration (build as static lib), update the macros accordingly.
- If `RandomGen` can't find `MazeGenerator.dll` at runtime, copy the DLL into the `RandomGen` executable output folder or configure project post-build events to place the DLL beside the exe.

How the renderer reads map data

- `App::OnUpdate` calls `GetTilePropertiesAtIndices(i, j)` (via the exported function implemented in the MazeGenerator side) to populate `m_tileProperties` (a 2D vector of `TileProperties`).
- The renderer creates Shader Resource Views (SRVs) for boxes/tiles using the tile properties and issues draw calls. The renderer uses compiled shaders (`*.cso`) located in `Resources/Shaders/`.

Suggested quick troubleshooting

- Build fails with unresolved externals for MazeGenerator symbols: check that the MazeGenerator project is built and the DLL import settings match (MAZEGENERATOR_EXPORTS define is set for the MazeGenerator project build).
- Graphics/shader errors: confirm shader `.cso` files exist in `Resources/Shaders` and are copied to the working directory or embedded as resources by the renderer.
- App starts but shows no maze: ensure `GenerateMap` is invoked (App calls `GenerateMap` in `LoadContent`) and rows/columns are within allowed ranges (clamped between 10 and 100 in the UI code).

Developer notes & small next steps

- The codebase is already modularly separated: the generator is reusable and could be packaged with a clearer C API header (the `MazeGenDefs.h` is a start). Consider adding a small sample console app that links to the DLL and prints a text-based map for quick headless tests.
- Add a simple CMake configuration to make building on non-MSVC toolchains easier (if desired). For now the repo uses Visual Studio project files.
- Add a post-build event to the MazeGenerator project to copy the built DLL into the RandomGen output folder to avoid manual copying.
- Add a small unit/integration test harness for the generator (e.g., run generator with fixed seed, verify expected number of passages / no isolated tiles) — this makes algorithm changes safer.

Files to inspect first when modifying behavior

- Change which algorithm is used / tweak options: `MazeGenerator/src/GridManager.cpp`, `MARecursiveBacktracker.*`, `MAEllers.*`.
- Add new rendering visuals: `RandomGen/Rendering/Renderer_D12.cpp`, `RandomGen/App.cpp` handles SRV creation.
- Modify room placement heuristics: `MazeGenerator/src/RoomGenerator.*`.

Contact / provenance

The project contains references in the root `README.md` to algorithm sources (Jamis Buck etc.) that were used as guidance for the implemented algorithms.

---

## Detailed Rendering and ImGui Architecture

This section provides an in-depth analysis of how the RandomGen rendering frontend works, including the DirectX 12 setup, ImGui integration, resource management, and the complete update/render loop flow.

### Application Lifecycle (App.cpp/App.h)

#### Class Structure

The `App` class (defined in [App.h](RandomGen/App.h)) is the main application controller with the following key members:

**Maze Parameters:**
- `m_rows`, `m_columns` - Grid dimensions (clamped 10-100)
- `m_generationType` - Full vs Step generation mode
- `m_mazeAlgorithm` - RecursiveBacktracker vs EllersAlgorithm

**Camera State:**
- `m_cameraPos` - World position (XMFLOAT3)
- `m_camAngles` - Pitch/yaw/roll angles (XMFLOAT3)
- `m_fov` - Field of view (degrees, clamped 12-90)

**Lighting:**
- `m_sunPos` - Sun position for shadow mapping (XMFLOAT4)

**Tile Data Cache:**
- `m_tileProperties` - 2D vector of `TileProperties` synced every frame from MazeGenerator DLL

**Timing:**
- `m_updateClock`, `m_renderClock` - High-resolution timers for delta time calculation

#### Initialization Flow

**App::Initialize()** ([App.cpp:102-124](RandomGen/App.cpp#L102-L124)):
1. Verifies DirectX Math CPU support via `DirectX::XMVerifyCPUSupportCheck()`
2. Allocates `m_tileProperties` 2D vector (rows × columns)
3. Creates global `GAME_WINDOW` (Window instance)
4. Registers App event callbacks with Window (OnUpdate, OnRender, input handlers)
5. Creates global `RENDERER` (Renderer_D12 instance)
6. Calls `RENDERER->PostInit()` to initialize DirectX 12 and ImGui
7. Calls `LoadContent()` to set up rendering resources
8. Shows the window and starts the Win32 message loop

**App::LoadContent()** ([App.cpp:126-141](RandomGen/App.cpp#L126-L141)):
1. Populates vertex buffer with box geometry (24 vertices for 6 faces)
2. Populates index buffer (36 indices for triangle list)
3. **Calls `GenerateMap()`** - triggers first maze generation from MazeGenerator DLL
4. Builds main rendering pipeline state (vertex_basic.cso, pixel_basic.cso)
5. Builds shadow mapping pipeline state (vertex_shadow.cso, pixel_shadow.cso)
6. Loads textures via DDS loader (Clay.dds, Grass.dds, Dirt.dds)
7. Resizes depth buffer to match window dimensions

#### Update/Render Loop

The application uses a **message-driven architecture**. The Windows message loop in [Window.cpp:329-336](RandomGen/Rendering/Window.cpp#L329-L336) processes messages, and `WM_PAINT` triggers both update and render.

**App::OnUpdate()** ([App.cpp:171-280](RandomGen/App.cpp#L171-L280)) - Called every frame:

**Frame Timing** (lines 171-190):
```cpp
m_updateClock.Tick();
double dt = m_updateClock.GetDeltaSeconds();
```

**Camera Matrix Calculation** (lines 193-198):
- Constructs quaternion from pitch/yaw/roll angles
- Extracts camera right, up, and forward vectors from orientation matrix

**Tile Data Synchronization** (lines 200-204) - **CRITICAL**:
```cpp
for (int i = 0; i < m_rows; ++i) {
    for (int j = 0; j < m_columns; ++j) {
        m_tileProperties[i][j] = GetTilePropertiesAtIndices(i, j);
    }
}
```
- Queries **every tile** from MazeGenerator DLL **every frame**
- This enables real-time visualization of step-by-step generation
- The DLL's `GetTilePropertiesAtIndices()` is thread-safe for reading

**SRV Creation** (line 206):
```cpp
RENDERER->CreateSRVForBoxes(m_tileProperties, m_rows, m_columns, 0);
```
- Transforms tile properties into GPU-ready instance data
- Generates floor and wall boxes based on tile passages
- Uploads model matrices and per-entity data to GPU

**ImGui Frame** (lines 208-259):
- Starts new ImGui frame (`ImGui_ImplDX12_NewFrame()`, `ImGui_ImplWin32_NewFrame()`, `ImGui::NewFrame()`)
- Builds "Controls" window with collapsing headers:
  - **Map Generation**: Algorithm combo, generation type combo, rows/columns inputs, regenerate button
  - **Atmosphere**: Sun position slider controlling time of day (6 AM - 6 PM)
- UI state changes immediately call DLL functions (`SetMazeGenerationAlgorithm()`, `SetMazeGenerationType()`)

**Input Handling** (lines 262-276):
- Only processes WASD camera movement if GUI is not capturing input
- Uses `GUIActive()` helper to check `ImGui::GetIO().WantCaptureMouse/Keyboard`

**MVP Update** (line 279):
```cpp
RENDERER->UpdateMVP(m_fov, m_cameraPos, camFwd, camRight, camUp, m_sunPos);
```
- Updates camera view-projection and sun view-projection matrices
- Writes directly to persistent-mapped constant buffer

**App::OnRender()** ([App.cpp:282-288](RandomGen/App.cpp#L282-L288)):
- Ticks render clock
- Calls `RENDERER->Render()` to execute DirectX 12 rendering

#### Event Handling

**OnKeyPressed/Released** ([App.cpp:290-300](RandomGen/App.cpp#L290-L300)):
- Updates global `Globals::INPUT_STATE.keyStates` array
- Forwards character input to ImGui via `ImGui_ImplWin32_AddInputCharacterUTF16()`

**OnMouseMoved** ([App.cpp:302-317](RandomGen/App.cpp#L302-L317)):
- Returns early if GUI is active (prevents camera rotation when dragging UI)
- Right mouse button drag rotates camera (updates `m_camAngles`)

**OnMouseButtonPressed/Released** ([App.cpp:319-336](RandomGen/App.cpp#L319-L336)):
- Updates `Globals::INPUT_STATE.mouseBtnState` bitfield

**OnMouseWheel** ([App.cpp:338-349](RandomGen/App.cpp#L338-L349)):
- Returns early if GUI is active
- Adjusts field of view (clamped 12-90 degrees)

**OnResize** ([App.cpp:351-361](RandomGen/App.cpp#L351-L361)):
- Updates App width/height
- Calls `RENDERER->ResizeTargets()` and `ResizeDepthBuffer()`

---

### Renderer_D12 Implementation

#### Class Architecture

The `Renderer_D12` class ([Renderer_D12.h](RandomGen/Rendering/Renderer_D12.h)) implements the DirectX 12 rendering backend with comprehensive GPU resource management.

**Key Data Structures:**

```cpp
struct VertexInput {
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 color;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 uv;  // xy = UV coords, z = face identifier
};

struct PerEntityData {
    unsigned int type;  // 0 = floor, 1 = wall
};

struct SceneData {
    DirectX::XMMATRIX camVP;   // Camera view-projection
    DirectX::XMMATRIX sunVP;   // Sun (shadow) view-projection
    DirectX::XMMATRIX PAD[2];  // 256-byte alignment padding
};

struct LightingData {
    DirectX::XMFLOAT4 sunPos;
    DirectX::XMFLOAT4 camPos;
    DirectX::XMFLOAT2 invShadowTexSize;  // 1.0 / shadow texture dimensions
};
```

#### DirectX 12 Initialization

**Renderer_D12 Constructor** ([Renderer_D12.cpp:182-213](RandomGen/Rendering/Renderer_D12.cpp#L182-L213)):
1. **EnableDebugLayer()**: Enables D3D12 debug layer in debug builds for validation
2. **CheckTearingSupport()**: Queries DXGI for variable refresh rate support
3. **GetAdapter()**: Selects best GPU (or WARP software adapter as fallback)
4. **CreateDevice()**: Creates `ID3D12Device2` with Feature Level 11.0
5. Creates **CommandQueue_D12** (direct command queue for rendering)
6. **CreateSwapChain()**: 3-buffer flip model swap chain with DXGI_FORMAT_R8G8B8A8_UNORM
7. Sets viewport and scissor rect to window dimensions
8. Determines highest supported root signature version (1.1 or 1.0)

**Renderer_D12::PostInit()** ([Renderer_D12.cpp:215-271](RandomGen/Rendering/Renderer_D12.cpp#L215-L271)):

**Descriptor Allocators** (lines 215-224):
```cpp
m_rtvAllocator = DescriptorAllocator_D12(RTV, 3);  // 3 back buffers
m_dsvAllocator = DescriptorAllocator_D12(DSV, 2);  // Main depth + shadow
m_shaderResourceAllocator = DescriptorAllocator_D12(CBV_SRV_UAV);
m_shaderResources = Allocate(7);  // Static allocation:
    // [0] Model transforms SRV
    // [1] SceneData CBV
    // [2] PerEntityData SRV
    // [3] Wall texture SRV
    // [4] Grass texture SRV
    // [5] Dirt texture SRV
    // [6] Shadow texture SRV
m_shaderResourceDynHeap = DynamicDescriptorHeap_D12(CBV_SRV_UAV);
```

**ImGui Initialization** (lines 226-266):
```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

ImGui_ImplWin32_Init(GAME_WINDOW->GetHandle());

// Create separate 64-descriptor heap for ImGui
D3D12_DESCRIPTOR_HEAP_DESC desc = {
    .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    .NumDescriptors = 64,
    .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
};
m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imGUISRVHeap));

// Custom allocator for ImGui SRV management
ImGui_ImplDX12_InitInfo init_info = {
    .Device = m_device.Get(),
    .CommandQueue = m_commQueue->GetD3D12CommandQueue().Get(),
    .NumFramesInFlight = 3,
    .RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
    .SrvDescriptorHeap = m_imGUISRVHeap.Get(),
    .SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*,
        D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
        return RENDERER->AllocateImGUIDescriptor(out_cpu_handle, out_gpu_handle);
    },
    .SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*,
        D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
        D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
        RENDERER->FreeImGUIDescriptor(cpu_handle, gpu_handle);
    }
};
ImGui_ImplDX12_Init(&init_info);
```

Uses `ExampleDescriptorHeapAllocator` (lines 61-100) - a free-list allocator for ImGui SRVs.

**UpdateRenderTargetViews()**: Creates RTVs for all 3 swap chain back buffers.

#### Pipeline State Setup

**IMPORTANT: Self-Describing Shader System**

The renderer now uses an **automatic shader reflection system** that eliminates hardcoded pipeline state. Shaders define their own resource requirements, and the pipeline is built automatically from shader bytecode.

**BuildPipelineState()** ([Renderer_D12.cpp:643-767](RandomGen/Rendering/Renderer_D12.cpp#L643-L767)):

**Automatic Shader Reflection Process:**
1. Load compiled shader bytecode (.cso files)
2. Use DirectX `ID3D12ShaderReflection` API to extract metadata:
   - Resource bindings (CBVs, SRVs, UAVs at t#/b# registers)
   - Vertex input signature (semantic names, formats)
   - Constant buffer layouts and sizes
3. Build root signature automatically from combined VS + PS requirements
4. Generate input layout automatically from vertex shader input signature
5. Static samplers still manually specified (not reflected from shaders)

**Implementation Files:**
- [ShaderReflection.h/cpp](RandomGen/Rendering/ShaderReflection.h) - Reflection wrapper and metadata structures
- [RootSignatureBuilder.h/cpp](RandomGen/Rendering/RootSignatureBuilder.h) - Automatic root signature generation

**Example of Automatic Root Signature (from current shaders):**
```cpp
// BEFORE (Manual - 80+ lines of hardcoded setup)
CD3DX12_ROOT_PARAMETER1 rootParameters[5];
rootParameters[0].InitAsConstantBufferView(0, 0, ...);
rootParameters[1].InitAsShaderResourceView(0, 0, ...);
// ... etc

// AFTER (Automatic - 6 lines)
ShaderReflector reflector;
reflector.ReflectShader(vertexShaderBlob.Get(), vsMetadata);
reflector.ReflectShader(pixelShaderBlob.Get(), psMetadata);
m_mainRootSigBuilder.AddShaderStage(vsMetadata);
m_mainRootSigBuilder.AddShaderStage(psMetadata);
m_mainRootSigBuilder.Build(m_device.Get(), rootSignature);
```

**Current Root Signature Layout (auto-generated):**
```
[0] CBV (b0) - SceneData (Visibility: VERTEX)
[1] SRV (t0) - ModelSB (Visibility: VERTEX)
[2] SRV (t1) - PerEntitySB (Visibility: PIXEL)
[3] Descriptor Table (t2-t5): wall, grass, dirt, shadow textures (Visibility: PIXEL)
[4] Root Constants (b0) - LightingPos (Visibility: PIXEL)

Static Samplers:
[s0] Point filter sampler
[s1] Comparison sampler (LESS_EQUAL)
```

**Key Benefits:**
- **Zero manual sync** - Modify shader bindings without touching C++ code
- **Automatic optimization** - Consecutive textures grouped into descriptor tables
- **Cross-stage merging** - VS and PS requirements combined with proper visibility flags
- **Validation** - Can verify resource bindings match shader expectations
- **Debug output** - `PrintLayout()` shows generated root signature structure

**Input Layout (auto-generated from vertex shader):**
- POSITION (R32G32B32_FLOAT)
- COLOR (R32G32B32_FLOAT)
- NORMAL (R32G32B32_FLOAT)
- TEXCOORD (R32G32B32_FLOAT)
- All attributes extracted automatically from shader input signature

**Pipeline State:**
- Primitive topology: Triangle list
- Depth format: D32_FLOAT
- RTV format: R8G8B8A8_UNORM
- Depth test enabled, depth write enabled
- Back-face culling enabled

**BuildShadowPipelineState()** ([Renderer_D12.cpp:745-837](RandomGen/Rendering/Renderer_D12.cpp#L745-L837)):
- Simpler root signature (just sun VP matrix and model transforms)
- Vertex shader only (no pixel shader output - depth-only pass)
- Back-face culling enabled to reduce shadow acne

#### Texture Loading

**LoadTextures()** ([Renderer_D12.cpp:477-514](RandomGen/Rendering/Renderer_D12.cpp#L477-L514)):

**Creates SceneData constant buffer:**
```cpp
m_vpBuffer = CreateBuffer(m_device, sizeof(SceneData), D3D12_HEAP_TYPE_UPLOAD);
D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {
    .BufferLocation = m_vpBuffer->GetGPUVirtualAddress(),
    .SizeInBytes = AlignUp(sizeof(SceneData), 256)
};
m_device->CreateConstantBufferView(&cbvDesc, m_shaderResources[1]);

// Persistent map for per-frame updates
CD3DX12_RANGE readRange(0, 0);
m_vpBuffer->Map(0, &readRange, &m_sceneDataBegin);
```

**Loads 3 DDS textures:**
```cpp
commandList->LoadTexture(L"Clay.dds", m_wallTexture);   // Descriptor 3
commandList->LoadTexture(L"Grass.dds", m_grassTexture); // Descriptor 4
commandList->LoadTexture(L"Dirt.dds", m_dirtTexture);   // Descriptor 5
```

Uses DirectXTex library via `CommandList_D12::LoadTexture()` - creates committed resource in DEFAULT heap, uploads via intermediate UPLOAD heap, generates mipmaps, transitions to PIXEL_SHADER_RESOURCE.

**Shadow texture** created in `ResizeDepthBuffer()` (descriptor 6) - same dimensions as main depth buffer.

#### Instance Data Generation

**CreateSRVForBoxes()** ([Renderer_D12.cpp:516-631](RandomGen/Rendering/Renderer_D12.cpp#L516-L631)):

This is the **critical function** that transforms tile properties into renderable geometry.

**Process:**

1. **Iterate through tiles** (lines 525-566):
```cpp
std::vector<XMMATRIX> mvpMatrices;
std::vector<PerEntityData> peds;

for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
        if (tiles[i][j].type == Empty) continue;  // Skip empty tiles

        float x = (j - columns / 2.0f) * 2.0f;  // Center grid
        float z = (i - rows / 2.0f) * 2.0f;

        // Create floor box
        XMMATRIX modelMat = XMMatrixTranslation(x, 0, z);
        mvpMatrices.push_back(modelMat);
        peds.push_back({ 0 });  // Type 0 = floor

        // Create wall boxes for blocked passages
        for (int p = 0; p < 4; ++p) {
            if (!(tiles[i][j].directions & DIRECTIONS[p])) {
                // Calculate wall position based on direction
                XMFLOAT3 offset = GetOffsetForDirection(DIRECTIONS[p]);
                XMMATRIX wallMat = XMMatrixScaling(1, 2, 1) *
                    XMMatrixTranslation(x + offset.x, 1, z + offset.z);
                mvpMatrices.push_back(wallMat);
                peds.push_back({ 1 });  // Type 1 = wall
            }
        }
    }
}
m_numInstances = mvpMatrices.size();
```

2. **Upload model transforms** (lines 572-599):
```cpp
commandList->UpdateBufferResource(m_device, &m_modelBuffer,
    mvpMatrices.size(), sizeof(XMMATRIX), mvpMatrices.data());

D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {
    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
    .Format = DXGI_FORMAT_UNKNOWN,
    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
    .Buffer.FirstElement = 0,
    .Buffer.NumElements = m_numInstances,
    .Buffer.StructureByteStride = sizeof(XMMATRIX)
};
m_device->CreateShaderResourceView(m_modelBuffer.Get(), &srvDesc,
    m_shaderResources[0]);
```

3. **Upload per-entity data** (lines 601-628):
- Same process for PerEntityData (type 0 or 1)
- Structured buffer at descriptor[2]

**Result**: Each tile generates 1 floor box + 0-4 wall boxes depending on passage directions.

#### Rendering Pipeline

**Render()** ([Renderer_D12.cpp:316-386](RandomGen/Rendering/Renderer_D12.cpp#L316-L386)):

**Shadow Pass** - `Shadowmap()` ([Renderer_D12.cpp:388-428](RandomGen/Rendering/Renderer_D12.cpp#L388-L428)):
```cpp
1. Get command list from queue
2. Transition shadow texture to DEPTH_WRITE
3. Clear depth to 1.0
4. Set shadow pipeline state and root signature
5. Bind vertex/index buffers (box geometry)
6. Set sun VP matrix as root constants
7. Bind model transform SRV (instance data)
8. DrawIndexedInstanced(36 indices, m_numInstances)  // Depth-only
9. Transition shadow texture to PIXEL_SHADER_RESOURCE
10. Execute command list and wait for completion (synchronous)
```

**Main Pass** ([Renderer_D12.cpp:316-386](RandomGen/Rendering/Renderer_D12.cpp#L316-L386)):
```cpp
1. Get command list from queue
2. Get current back buffer index
3. Transition back buffer to RENDER_TARGET
4. Clear render target (sky blue: 0.4f, 0.6f, 1.0f, 1.0f)
5. Clear main depth buffer to 1.0
6. Set main pipeline state and root signature
7. Set primitive topology (triangle list)
8. Bind vertex/index buffers (box geometry)
9. Set viewport and scissor rect
10. Bind render target and depth stencil
11. Set root parameters:
    [0] SceneData CBV (camera + sun VP)
    [1] Model transforms SRV
    [2] PerEntity data SRV
    [3] Descriptor table (4 textures via dynamic heap)
    [4] LightingData constants (sun pos, cam pos, shadow tex size)
12. DrawIndexedInstanced(36 indices, m_numInstances)
13. Render ImGui (lines 365-370):
    - Set ImGui descriptor heap
    - ImGui::Render()
    - ImGui_ImplDX12_RenderDrawData()
14. Transition back buffer to PRESENT
15. Execute command list
16. Present swap chain (VSync or tearing mode)
17. Wait for frame fence completion
18. Advance to next back buffer index
19. Reset dynamic descriptor heap
20. Increment frame counter
```

**UpdateMVP()** ([Renderer_D12.cpp:901-925](RandomGen/Rendering/Renderer_D12.cpp#L901-L925)):
- Constructs camera view-projection matrix from FOV, position, and orientation vectors
- Constructs orthographic shadow view-projection matrix:
  - Centers shadow camera on world center (rows/2, columns/2)
  - Looks along sun direction
  - Uses orthographic projection sized to cover entire maze
- Updates persistent-mapped constant buffer via `memcpy(m_sceneDataBegin, &m_sceneData, sizeof(m_sceneData))`

#### Resource Management

**Descriptor Allocation:**
- **RTVs**: Static allocation for 3 back buffers (CPU-visible only)
- **DSVs**: Static allocation for main depth buffer + shadow map (CPU-visible only)
- **SRVs**: Static allocation for 7 descriptors (1 CBV, 2 structured buffers, 4 textures) - shader-visible
- **ImGui SRVs**: Separate shader-visible heap with free-list allocator (64 descriptors)

**Dynamic Descriptor Heap:**
- Used for binding texture descriptor table in pixel shader
- Parses root signature to determine which parameters need dynamic descriptors
- Stages descriptors to CPU-visible staging area
- Commits to GPU-visible heap before draw call
- Reset each frame

**Memory Types:**
- **Vertex/Index buffers**: DEFAULT heap (GPU-only) uploaded via intermediate UPLOAD heap
- **Constant buffer**: UPLOAD heap (CPU-visible, persistent map) for per-frame updates
- **Model/Entity buffers**: DEFAULT heap, updated each frame via intermediate UPLOAD heap
- **Textures**: DEFAULT heap, loaded from DDS files via intermediate UPLOAD heap

---

### ImGui Integration Details

#### Initialization

**Where**: `Renderer_D12::PostInit()`, lines 226-266 ([Renderer_D12.cpp:226-266](RandomGen/Rendering/Renderer_D12.cpp#L226-L266))

**ImGui Context Setup:**
```cpp
IMGUI_CHECKVERSION();
ImGui::CreateContext();
ImGuiIO& io = ImGui::GetIO();
io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;  // Enables docking features
```

**Win32 Backend:**
```cpp
ImGui_ImplWin32_Init(GAME_WINDOW->GetHandle());
```
- Integrates with Win32 message handling
- `ImGui_ImplWin32_WndProcHandler()` called in Window WndProc (lines 37-41)

**DirectX 12 Backend:**
```cpp
ImGui_ImplDX12_InitInfo init_info = {
    .Device = m_device.Get(),
    .CommandQueue = m_commQueue->GetD3D12CommandQueue().Get(),
    .NumFramesInFlight = NUM_BACKBUFFER_FRAMES,  // 3
    .RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
    .SrvDescriptorHeap = m_imGUISRVHeap.Get()
};
ImGui_ImplDX12_Init(&init_info);
```

**Custom Allocator:**
- ImGui requires dynamic SRV allocation for textures (fonts, user images)
- Uses `ExampleDescriptorHeapAllocator` class with free-list management
- 64 descriptors available in separate shader-visible heap
- Allocator/free callbacks are lambdas capturing global `RENDERER` pointer

#### UI Controls

**Map Generation Section** ([App.cpp:213-236](RandomGen/App.cpp#L213-L236)):

```cpp
ImGui::Combo("Maze Algorithm", &m_mazeAlgorithm,
    MazeDefs::MazeAlgorithmLabels, 2);
SetMazeGenerationAlgorithm(MazeDefs::MazeAlgorithm(m_mazeAlgorithm));
```
- **Options**: RecursiveBacktracker (0), EllersAlgorithm (1)
- Immediately calls DLL function to update algorithm

```cpp
ImGui::Combo("Generation Type", &m_generationType,
    MazeDefs::GenerateTypeLabels, 2);
SetMazeGenerationType(MazeDefs::GenerateType(m_generationType));
```
- **Options**: Full (0 - generate to completion), Step (1 - incremental visualization)
- Controls threading and synchronization in GridManager

```cpp
ImGui::InputInt("Rows", &m_rows);
m_rows = std::clamp(m_rows, 10, 100);
ImGui::InputInt("Columns", &m_columns);
m_columns = std::clamp(m_columns, 10, 100);
```
- Clamped to 10-100 range for performance

```cpp
bool regen = ImGui::Button("Regenerate");
if (regen) {
    m_tileProperties.resize(m_rows);
    for (int i = 0; i < m_rows; ++i) {
        m_tileProperties[i].resize(m_columns);
    }
    GenerateMap(m_width, m_height, m_rows, m_columns);
}
```
- Resizes tile cache and triggers new generation

**Atmosphere Section** ([App.cpp:238-256](RandomGen/App.cpp#L238-L256)):

```cpp
float angle = GetAngleOnZYPlane(m_sunPos);
float timeOfDay = ((angle / M_PI) + 1) * 12.f;
ImGui::SliderFloat("Sun Angle 6AM-6PM", &timeOfDay, 6, 18);
angle = ((timeOfDay / 12.f) - 1.f) * M_PI;
m_sunPos = GetPositionFromAngle(angle, radius);
```
- Controls sun position on ZY plane arc (radius 30 units)
- Maps to intuitive time of day (6 AM - 6 PM)
- Directly affects shadow direction in rendering

#### UI to Generation Data Flow

```
User modifies ImGui control
    ↓
Local variable updated (m_mazeAlgorithm, m_rows, etc.)
    ↓
Algorithm/Type changes → Call DLL immediately
    SetMazeGenerationAlgorithm()
    SetMazeGenerationType()
    ↓
Regenerate button → GenerateMap(width, height, rows, columns)
    ↓
MazeGenerator DLL starts generation thread (Step) or blocks (Full)
    ↓
Next frame OnUpdate():
    Query all tiles via GetTilePropertiesAtIndices()
    ↓
    CreateSRVForBoxes() transforms tiles to GPU instances
    ↓
    Render() draws updated maze
```

#### ImGui Rendering Flow

**Per-Frame:**

1. **OnUpdate()** ([App.cpp:208-259](RandomGen/App.cpp#L208-L259)):
```cpp
ImGui_ImplDX12_NewFrame();
ImGui_ImplWin32_NewFrame();
ImGui::NewFrame();

// Build UI (recording draw commands)
ImGui::Begin("Controls");
// ... UI construction ...
ImGui::End();
```

2. **OnRender()** → **Renderer_D12::Render()** ([Renderer_D12.cpp:365-370](RandomGen/Rendering/Renderer_D12.cpp#L365-L370)):
```cpp
commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
    m_imGUISRVHeap.Get());
ImGui::Render();  // Finalize draw data
ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(),
    commandList->GetGraphicsCommandList().Get());
```

**Integration Points:**
- ImGui draw commands submitted to same command list as main rendering
- Rendered **after** the maze, so UI appears on top
- Uses separate descriptor heap (swapped before ImGui render)
- ImGui manages its own vertex/index buffers and pipeline state

#### Input Handling

**Window.cpp WndProc** ([Window.cpp:37-41](RandomGen/Rendering/Window.cpp#L37-L41)):
```cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParam, lParam))
    return true;  // ImGui consumed the message
```
- ImGui processes message first
- If consumed, app doesn't handle it

**GUIActive() Helper** ([App.cpp:95-101](RandomGen/App.cpp#L95-L101)):
```cpp
inline bool GUIActive() {
    auto& io = ImGui::GetIO();
    return io.WantCaptureMouse || io.WantCaptureKeyboard;
}
```
- Prevents camera control when hovering/typing in UI
- Checked before processing WASD/mouse input in OnUpdate()

---

### Shader Pipeline

#### Vertex Shader (vertex_basic.hlsl)

**Inputs:**
```hlsl
struct VertexInput {
    float3 position : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL0;
    float3 uv : TEXCOORD0;
    uint instanceid : SV_InstanceID;
};
```

**Resources:**
```hlsl
StructuredBuffer<Model> ModelSB : register(t0);  // Per-instance transforms
ConstantBuffer<SceneData> SceneDataCB : register(b0);  // Camera/Sun VP
```

**Vertex Transformation:**
```hlsl
float4 worldPos = mul(ModelSB[instanceid].M, float4(position, 1.0));
output.hpos = mul(SceneDataCB.camVP, worldPos);  // Clip space
output.sunPos = mul(SceneDataCB.sunVP, worldPos);  // Shadow space
output.worldPos = worldPos.xyz;
output.normal = mul((float3x3)ModelSB[instanceid].M, normal);
```

#### Pixel Shader (pixel_basic.hlsl)

**Inputs:**
```hlsl
struct PixelInput {
    float4 hpos : SV_POSITION;
    float3 worldPos : POSITION;
    float3 color : COLOR;
    float3 normal : NORMAL0;
    float3 uv : TEXCOORD0;
    float4 sunPos : TEXCOORD1;  // Shadow map coordinates
    uint instanceid : INSTANCE;
};
```

**Resources:**
```hlsl
StructuredBuffer<PerEntityData> PerEntitySB : register(t1);
Texture2D wallTexture : register(t2);
Texture2D grassTexture : register(t3);
Texture2D dirtTexture : register(t4);
Texture2D shadowTexture : register(t5);
SamplerState TextureSampler : register(s0);
SamplerComparisonState ShadowSampler : register(s1);
```

**Lighting Constants:**
```hlsl
cbuffer LightingData : register(b0) {
    float4 sunPos;
    float4 camPos;
    float2 invShadowTexSize;
};
```

**Texture Selection** (lines 69-83):
```hlsl
if (input.uv.z > 0.5) {  // Top face (z coordinate distinguishes faces)
    if (ped.type != 1) {  // Floor
        color = grassTexture.Sample(TextureSampler, input.uv.xy);
    } else {  // Wall
        color = wallTexture.Sample(TextureSampler, input.uv.xy);
    }
} else {  // Side/bottom faces
    color = dirtTexture.Sample(TextureSampler, input.uv.xy);
}
```

**Shadow Mapping** (lines 85-115 with PCF):
```hlsl
// Transform to shadow texture coordinates
float2 shadowTexCoord = input.sunPos.xy / input.sunPos.w * 0.5 + 0.5;
shadowTexCoord.y = 1.0 - shadowTexCoord.y;
float depth = input.sunPos.z / input.sunPos.w;

// 9-tap PCF (Percentage Closer Filtering)
float shadow = 0.0;
for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
        float2 offset = float2(x, y) * invShadowTexSize;
        shadow += shadowTexture.SampleCmpLevelZero(
            ShadowSampler,
            shadowTexCoord + offset,
            depth - 0.001  // Bias to reduce shadow acne
        );
    }
}
shadow /= 9.0;  // Average
```

**Blinn-Phong Lighting:**
```hlsl
float3 lightDir = normalize(sunPos.xyz - input.worldPos);
float3 viewDir = normalize(camPos.xyz - input.worldPos);
float3 halfDir = normalize(lightDir + viewDir);

float3 ambient = color.rgb * 0.3;
float3 diffuse = color.rgb * max(0.0, dot(normal, lightDir)) * 0.7;
float3 specular = float3(1, 1, 1) * pow(max(0.0, dot(normal, halfDir)), 32) * 0.3;

float3 finalColor = (ambient + (diffuse + specular) * shadow);
return float4(finalColor, 1.0);
```

---

### Complete Frame Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│ Win32 Message Loop                                      │
├─────────────────────────────────────────────────────────┤
│ PeekMessage() / DispatchMessage()                       │
└─────────────────────────────────────────────────────────┘
                         ↓
         ┌───────────────────────────┐
         │ WM_PAINT message received │
         └───────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│ Window::OnUpdate() → App::OnUpdate()                    │
├─────────────────────────────────────────────────────────┤
│ 1. Frame Timing                                         │
│    • Tick update clock, calculate delta time            │
│                                                         │
│ 2. Camera Matrix Calculation                            │
│    • Build quaternion from angles                       │
│    • Extract right/up/forward vectors                   │
│                                                         │
│ 3. Tile Data Sync (CRITICAL)                            │
│    • Query every tile from MazeGenerator DLL            │
│    • Enables step-by-step visualization                 │
│                                                         │
│ 4. GPU Instance Data Generation                         │
│    • CreateSRVForBoxes() transforms tiles to instances  │
│    • 1 floor + 0-4 walls per tile                       │
│    • Upload to GPU structured buffers                   │
│                                                         │
│ 5. ImGui Frame                                          │
│    • NewFrame() for Win32, DX12, and ImGui              │
│    • Build UI (Map Generation + Atmosphere controls)    │
│                                                         │
│ 6. Input Processing                                     │
│    • WASD camera movement (if !GUIActive())             │
│    • Mouse rotation, wheel FOV                          │
│                                                         │
│ 7. MVP Matrix Update                                    │
│    • UpdateMVP() builds camera + sun VP matrices        │
│    • memcpy to persistent-mapped constant buffer        │
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│ Window::OnRender() → App::OnRender() → Renderer::Render│
├─────────────────────────────────────────────────────────┤
│ SHADOW PASS (Shadowmap())                               │
│ 1. Get command list                                     │
│ 2. Transition shadow texture → DEPTH_WRITE              │
│ 3. Clear shadow depth                                   │
│ 4. Set shadow pipeline state                            │
│ 5. Bind vertex/index buffers                            │
│ 6. Set sun VP + model SRV                               │
│ 7. DrawIndexedInstanced (depth-only)                    │
│ 8. Transition shadow texture → PIXEL_SHADER_RESOURCE    │
│ 9. Execute + wait (synchronous)                         │
├─────────────────────────────────────────────────────────┤
│ MAIN PASS                                               │
│ 1. Get command list + back buffer                       │
│ 2. Transition back buffer → RENDER_TARGET               │
│ 3. Clear render target (sky blue) + depth               │
│ 4. Set main pipeline state + root signature             │
│ 5. Bind vertex/index buffers                            │
│ 6. Set viewport + scissor + render targets              │
│ 7. Set root parameters:                                 │
│    - [0] SceneData CBV                                  │
│    - [1] Model transforms SRV                           │
│    - [2] PerEntity data SRV                             │
│    - [3] Texture descriptor table                       │
│    - [4] LightingData constants                         │
│ 8. DrawIndexedInstanced (maze geometry)                 │
│ 9. Set ImGui descriptor heap                            │
│ 10. ImGui::Render() + RenderDrawData()                  │
│ 11. Transition back buffer → PRESENT                    │
│ 12. Execute command list                                │
│ 13. Present swap chain                                  │
│ 14. Wait for frame fence                                │
│ 15. Advance to next back buffer                         │
│ 16. Reset dynamic descriptor heap                       │
└─────────────────────────────────────────────────────────┘
```

---

### Key Files Reference

#### Core Application
- [main.cpp](RandomGen/main.cpp) - Entry point, command-line parsing, App creation
- [App.h](RandomGen/App.h) - App class declaration
- [App.cpp](RandomGen/App.cpp) - App implementation (lifecycle, update/render, ImGui)
- [AppDefs.h](RandomGen/AppDefs.h) - Global state, startup values, input tracking

#### Rendering
- [Renderer_D12.h](RandomGen/Rendering/Renderer_D12.h) - Renderer class, data structures
- [Renderer_D12.cpp](RandomGen/Rendering/Renderer_D12.cpp) - DirectX 12 implementation (947 lines)
- [Window.h](RandomGen/Rendering/Window.h) - Window wrapper
- [Window.cpp](RandomGen/Rendering/Window.cpp) - Win32 message handling
- [CommandQueue_D12.h](RandomGen/Rendering/CommandQueue_D12.h) - Command queue wrapper
- [CommandList_D12.h](RandomGen/Rendering/CommandList_D12.h) - Command list wrapper
- [DescriptorAllocator_D12.h](RandomGen/Rendering/DescriptorAllocator_D12.h) - Descriptor allocation
- [DynamicDescriptorHeap_D12.h](RandomGen/Rendering/DynamicDescriptorHeap_D12.h) - Dynamic descriptor binding

#### Shaders
- [vertex_basic.hlsl](RandomGen/Resources/Shaders/vertex_basic.hlsl) - Main vertex shader
- [pixel_basic.hlsl](RandomGen/Resources/Shaders/pixel_basic.hlsl) - Main pixel shader (lighting + shadows)
- [vertex_shadow.hlsl](RandomGen/Resources/Shaders/vertex_shadow.hlsl) - Shadow pass vertex shader
- [pixel_shadow.hlsl](RandomGen/Resources/Shaders/pixel_shadow.hlsl) - Shadow pass pixel shader

#### Resources
- [Clay.dds](RandomGen/Resources/Clay.dds) - Wall texture
- [Grass.dds](RandomGen/Resources/Grass.dds) - Floor (top) texture
- [Dirt.dds](RandomGen/Resources/Dirt.dds) - Side/bottom texture

#### Utilities
- [Events.h](RandomGen/Events.h) - Event args structures
- [HighResolutionClock.h](RandomGen/HighResolutionClock.h) - Frame timing

---

## Architecture Patterns and Best Practices

### Event-Driven Architecture
- Win32 message pump drives updates and rendering
- Event args structs carry state (mouse position, key codes, delta time)
- Callback pattern for App to handle Window events

### Global Singletons
- `GAME_WINDOW` and `RENDERER` are global shared_ptrs
- Allows easy access from ImGui callbacks and utility functions
- Initialized once in `App::Initialize()`

### Triple-Buffering
- 3 swap chain back buffers for smooth presentation
- Per-frame fence values track GPU progress
- CPU waits for specific frame completion before reusing resources

### Command List Pooling
- `CommandQueue_D12` manages command list allocation and recycling
- Command lists reset and reused each frame
- Intermediate resources tracked to keep alive until GPU finishes

### Descriptor Management Strategy
- **Static allocation** for known resources (7 main SRVs/CBVs)
- **Dynamic allocation** for ImGui textures (free-list allocator)
- **Dynamic descriptor heap** for per-draw descriptor table binding

### Persistent Mapping
- Constant buffers kept mapped for entire application lifetime
- Updated via `memcpy` each frame
- Avoids map/unmap overhead (UPLOAD heap)

### Instance Rendering
- Single draw call for entire maze using `DrawIndexedInstanced`
- Per-instance data in structured buffers (model matrices, entity types)
- Minimizes CPU-GPU overhead

### Shadow Mapping
- Separate depth-only pass with orthographic projection
- 9-tap PCF filtering for soft shadows
- Comparison sampler with LESS_EQUAL comparison function

---

## Performance Optimization: Dirty Flag System

The project implements a comprehensive dirty flag optimization system that eliminates redundant tile queries and GPU uploads, achieving 99%+ reduction in unnecessary work.

### Problem Statement

**Original Implementation Issues:**
- Step mode queried ALL tiles EVERY frame (60fps)
- For 50×50 maze: 150,000 tile queries/second
- Actual tile changes: ~100/second
- **Waste ratio: 99.93%** - CPU spending cycles on redundant DLL calls and memory copies

### Solution Architecture

The optimization uses a **granular dirty flag system** combined with **frame rate throttling** for Step mode visualization.

#### Core Components

**1. Dirty Flag Tracking ([GridManager.h:94](MazeGenerator/src/GridManager.h#L94))**
```cpp
std::atomic<bool> m_isDirty = true;  // Thread-safe flag, starts dirty for first render

bool IsDirty() const { return m_isDirty; }
void ClearDirtyFlag() { m_isDirty = false; }
void SetDirtyFlag() { m_isDirty = true; }
```

**2. Callback Infrastructure ([IThreadedSolver.h:30-55](MazeGenerator/src/IThreadedSolver.h#L30-L55))**
```cpp
std::function<void()> m_dirtyCallback;

void SetDirtyCallback(std::function<void()> callback) {
    m_dirtyCallback = callback;
}

void NotifyTilesModified() {
    if (m_dirtyCallback) {
        m_dirtyCallback();  // Calls GridManager::SetDirtyFlag()
    }
}
```

All generation components (`IMazeAlgorithm`, `MazeConnector`, `DeadEndRemover`) inherit from `IThreadedSolver` and gain access to this notification system.

**3. Algorithm Integration**

Each algorithm calls `NotifyTilesModified()` after modifying tiles:

- **MARecursiveBacktracker** ([MARecursiveBacktracker.cpp:155](MazeGenerator/src/MARecursiveBacktracker.cpp#L155)): After carving passages
- **MAEllers** ([MAEllers.cpp](MazeGenerator/src/MAEllers.cpp)): After row initialization, horizontal merges, and vertical cuts
- **MazeConnector** ([MazeConnector.cpp](MazeGenerator/src/MazeConnector.cpp)): After opening room connections
- **DeadEndRemover** ([DeadEndRemover.cpp:67](MazeGenerator/src/DeadEndRemover.cpp#L67)): After removing dead end tiles

**4. Rendering Query Logic ([App.cpp:200-242](RandomGen/App.cpp#L200-L242))**

```cpp
// Check if maze is dirty (tiles have changed)
if (IsMazeDirty()) {
    // In Step mode, also check frame rate throttling
    if (m_generationType == MazeDefs::Step) {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastStepQuery).count();
        if (elapsed >= 16) {  // ~60fps max
            shouldQuery = true;
        }
    } else {
        // Full mode: query immediately when dirty
        shouldQuery = true;
    }
}

if (shouldQuery) {
    // Batch API: single DLL call for all tiles
    GetAllTileProperties(tempBuffer.data(), m_rows * m_columns);
    // Upload to GPU
    RENDERER->CreateSRVForBoxes(m_tileProperties, m_rows, m_columns, 0);
    // Clear flag after sync
    ClearMazeDirtyFlag();
}
```

### Performance Characteristics

#### Full Mode (Instant Generation)
**Before Optimization:**
- Queries every frame even after generation completes
- 60+ unnecessary queries per second on static maze

**After Optimization:**
- Dirty flag set once after generation completes
- Single query + GPU upload
- Flag cleared, no further queries
- **Result: 0 wasted frames** (100% efficiency)

#### Step Mode (Animated Visualization)
**Before Optimization:**
- 60 fps × 2,500 tiles = 150,000 queries/sec
- 99.93% waste (tiles change ~100 times/sec)

**After Optimization:**
- Dirty flag set after each tile modification (every 5ms in algorithm)
- Frame rate throttle limits queries to 60/sec max
- Only queries when: `IsMazeDirty() == true` AND `elapsed >= 16ms`
- **Result: ~60 queries/sec, only when tiles actually changed** (99%+ efficiency gain)

### Callback Flow Diagram

```
Algorithm modifies tiles
    ↓
NotifyTilesModified() called
    ↓
m_dirtyCallback() → GridManager::SetDirtyFlag()
    ↓
m_isDirty = true (atomic, thread-safe)
    ↓
App::OnUpdate() checks IsMazeDirty()
    ↓
[Step mode: Also check frame throttle]
    ↓
Query all tiles via batch API
    ↓
Upload to GPU (CreateSRVForBoxes)
    ↓
ClearMazeDirtyFlag()
    ↓
m_isDirty = false (wait for next modification)
```

### Thread Safety

- **Dirty flag:** `std::atomic<bool>` for lock-free thread-safe reads/writes
- **Callbacks:** Set before generation starts in `GridManager` worker threads
- **Algorithms:** Call `NotifyTilesModified()` from generation threads
- **Rendering:** Checks `IsMazeDirty()` from main thread (render loop)
- **No locks required** for dirty flag access (atomic operations)

### Batch API Benefits

**Beyond Dirty Flags:** The batch API (`GetAllTileProperties`) provides additional benefits:

1. **Single DLL Boundary Crossing**
   - Before: 10,000 calls for 100×100 maze
   - After: 1 call per frame (when dirty)

2. **Contiguous Memory Access**
   - Sequential row-major iteration in DLL
   - Better CPU cache utilization
   - Predictable memory access pattern

3. **Reduced Function Call Overhead**
   - Eliminates per-tile function prologue/epilogue
   - Single setup cost amortized over all tiles

### Dynamic Tile Updates (Post-Generation)

If tiles need modification after generation (e.g., for gameplay mechanics):

**Option 1: Manual Dirty Flag (Recommended)**
```cpp
// In your game code after modifying tiles:
ModifyTileAtPosition(x, y, newProperties);
SetMazeDirty();  // Would need to be exposed in public API
```

**Option 2: Always Query in Dynamic Mode**
- Bypass dirty check if tiles can change externally
- Still use batch API for efficiency
- Only recommended if changes are frequent

**Current Status:** The API does not expose `SetMazeDirty()` publicly. The optimization assumes mazes are static after generation completes in Full mode, which matches the current use case (procedural generation, no post-generation modification).

### Implementation Files

**Core Infrastructure:**
- [IThreadedSolver.h](MazeGenerator/src/IThreadedSolver.h) - Base class with callback system
- [IMazeAlgorithm.h](MazeGenerator/src/IMazeAlgorithm.h) - Maze algorithm interface (inherits callbacks)
- [GridManager.h](MazeGenerator/src/GridManager.h) - Dirty flag storage
- [GridManager.cpp](MazeGenerator/src/GridManager.cpp) - Callback registration for all phases
- [MazeGenDefs.h](MazeGenerator/include/MazeGenDefs.h) - Public DLL API with dirty flag functions

**Algorithm Implementations:**
- [MARecursiveBacktracker.cpp](MazeGenerator/src/MARecursiveBacktracker.cpp) - Backtracker with dirty notifications
- [MAEllers.cpp](MazeGenerator/src/MAEllers.cpp) - Eller's algorithm with dirty notifications
- [MazeConnector.cpp](MazeGenerator/src/MazeConnector.cpp) - Room connector with dirty notifications
- [DeadEndRemover.cpp](MazeGenerator/src/DeadEndRemover.cpp) - Dead end remover with dirty notifications

**Rendering Integration:**
- [App.cpp](RandomGen/App.cpp) - Query logic with dirty check and frame throttling

---

## Self-Describing Shader Pipeline System

### Overview

The RandomGen renderer implements an **automatic shader reflection system** that eliminates hardcoded pipeline state setup. Shaders define their own resource requirements through standard HLSL declarations, and the DirectX 12 pipeline is built automatically from compiled shader bytecode.

This system was implemented to solve the maintenance problem of keeping C++ resource bindings synchronized with shader code. Any change to shader resources (adding textures, changing buffer bindings, modifying vertex attributes) previously required manual updates to multiple C++ code sections, leading to errors and tedious debugging.

### Architecture

#### Core Components

**1. ShaderReflection ([ShaderReflection.h/cpp](RandomGen/Rendering/ShaderReflection.h))**

Wraps DirectX `ID3D12ShaderReflection` API to extract metadata from compiled shader bytecode:

**Metadata Structures:**
```cpp
struct ShaderResourceBinding {
    std::string name;                    // Resource name (e.g., "ModelSB")
    D3D_SHADER_INPUT_TYPE type;          // CBV, SRV, UAV, Sampler
    UINT bindPoint;                      // Register slot (t0, b0, s0, etc.)
    UINT bindCount;                      // Array size
    UINT space;                          // Register space
    D3D12_SHADER_VISIBILITY visibility;  // VERTEX, PIXEL, or ALL
};

struct ShaderInputElement {
    std::string semanticName;            // POSITION, TEXCOORD, etc.
    UINT semanticIndex;                  // Semantic index (0 for POSITION0)
    DXGI_FORMAT format;                  // Data format (R32G32B32_FLOAT, etc.)
    UINT inputSlot;                      // Vertex buffer slot
    UINT alignedByteOffset;              // Offset in vertex structure
};

struct ShaderConstantBuffer {
    std::string name;                    // CB name
    UINT bindPoint;                      // Register (b#)
    UINT size;                           // Size in bytes
    UINT variableCount;                  // Number of variables inside
};
```

**ShaderReflector Class:**
```cpp
class ShaderReflector {
public:
    // Reflect compiled shader blob and extract all metadata
    bool ReflectShader(ID3DBlob* shaderBlob, ShaderMetadata& outMetadata);

private:
    void ReflectResources(ID3D12ShaderReflection*, ShaderMetadata&);
    void ReflectInputSignature(ID3D12ShaderReflection*, ShaderMetadata&);
    void ReflectConstantBuffers(ID3D12ShaderReflection*, ShaderMetadata&);
    D3D12_SHADER_VISIBILITY GetVisibilityFromShaderType(UINT shaderType);
};
```

**Key Features:**
- Uses `D3DReflect()` to create reflection interface from bytecode
- Iterates through all bound resources (CBVs, SRVs, UAVs, samplers)
- Extracts vertex input signature (skips system values like SV_InstanceID)
- Determines shader visibility automatically from shader type (VS → VERTEX, PS → PIXEL)
- Converts D3D reflection types to DXGI formats for input layout

**2. RootSignatureBuilder ([RootSignatureBuilder.h/cpp](RandomGen/Rendering/RootSignatureBuilder.h))**

Generates DirectX 12 root signatures automatically from shader metadata:

```cpp
class RootSignatureBuilder {
public:
    // Add shader stage metadata (can add multiple: VS, PS, etc.)
    void AddShaderStage(const ShaderMetadata& metadata);

    // Add static samplers (not reflected from shaders)
    void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler);

    // Build the final root signature
    bool Build(ID3D12Device* device, ComPtr<ID3D12RootSignature>& outRootSignature);

    // Get resource-to-root-parameter mappings
    const std::vector<ResourceMapping>& GetResourceMappings() const;

    // Debug output
    void PrintLayout() const;

private:
    std::vector<RootParameterInfo> m_rootParameters;
    std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
    std::vector<ResourceMapping> m_resourceMappings;
    std::vector<ShaderResourceBinding> m_allBindings;

    void MergeResourceBinding(const ShaderResourceBinding& binding);
    void OptimizeDescriptorTables();
};
```

**Smart Features:**
- **Cross-stage merging**: Combines VS and PS resource requirements
  - Same resource at same register → single root parameter with merged visibility
  - Different resources → separate root parameters
- **Automatic descriptor table packing**: Groups consecutive textures (t2, t3, t4, t5 → single descriptor table)
- **Root parameter type selection**:
  - CBVs → Root descriptor (direct GPU address, faster)
  - Structured buffers → Root SRV descriptor
  - Textures → Descriptor table (multiple textures grouped)
  - Root constants → 32-bit constants (fastest access)
- **Visibility optimization**: Sets proper shader visibility flags to improve GPU performance

### Usage Example

**Before (Manual - 80+ lines):**
```cpp
void Renderer_D12::BuildPipelineState() {
    // Load shaders
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    D3DReadFileToBlob(L"vertex_basic.cso", &vertexShaderBlob);
    D3DReadFileToBlob(L"pixel_basic.cso", &pixelShaderBlob);

    // Manually define input layout
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, ... },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, ... },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, ... },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, ... },
    };

    // Manually build root signature
    CD3DX12_ROOT_PARAMETER1 rootParameters[5];
    rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[2].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_DESCRIPTOR_RANGE1 texture1Range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 2);
    rootParameters[3].InitAsDescriptorTable(1, &texture1Range, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[4].InitAsConstants(sizeof(LightingData) / 4, 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

    // Define static samplers
    D3D12_STATIC_SAMPLER_DESC samplers[2];
    // ... 30+ lines of sampler setup ...

    // Create root signature
    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, 2, samplers, ...);
    // ... serialize and create ...
}
```

**After (Automatic - 30 lines, mostly comments):**
```cpp
void Renderer_D12::BuildPipelineState() {
    // Load shaders
    ComPtr<ID3DBlob> vertexShaderBlob, pixelShaderBlob;
    D3DReadFileToBlob(L"vertex_basic.cso", &vertexShaderBlob);
    D3DReadFileToBlob(L"pixel_basic.cso", &pixelShaderBlob);

    // Reflect shaders to extract metadata
    Rendering::ShaderReflector reflector;
    reflector.ReflectShader(vertexShaderBlob.Get(), m_vertexShaderMetadata);
    reflector.ReflectShader(pixelShaderBlob.Get(), m_pixelShaderMetadata);

    // Build input layout automatically from vertex shader
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
    for (const auto& element : m_vertexShaderMetadata.inputElements) {
        D3D12_INPUT_ELEMENT_DESC desc = {};
        desc.SemanticName = element.semanticName.c_str();
        desc.SemanticIndex = element.semanticIndex;
        desc.Format = element.format;
        desc.InputSlot = element.inputSlot;
        desc.AlignedByteOffset = element.alignedByteOffset;
        desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        desc.InstanceDataStepRate = 0;
        inputLayout.push_back(desc);
    }

    // Build root signature automatically from shader metadata
    m_mainRootSigBuilder.AddShaderStage(m_vertexShaderMetadata);
    m_mainRootSigBuilder.AddShaderStage(m_pixelShaderMetadata);

    // Static samplers (not reflected) still added manually
    m_mainRootSigBuilder.AddStaticSampler(samplers[0]);
    m_mainRootSigBuilder.AddStaticSampler(samplers[1]);

    // Build - automatically matches shader requirements!
    ComPtr<ID3D12RootSignature> d3d12RootSig;
    m_mainRootSigBuilder.Build(m_device.Get(), d3d12RootSig);

    // Debug: Print generated layout
    m_mainRootSigBuilder.PrintLayout();

    // Use in pipeline state
    pipelineStateStream.pRootSignature = d3d12RootSig.Get();
    pipelineStateStream.InputLayout = { inputLayout.data(), (UINT)inputLayout.size() };
    // ... rest of pipeline state ...
}
```

### Workflow: Adding a New Texture

**Example: Add a new texture at t6 for a detail map**

**1. Modify HLSL shader only:**
```hlsl
// pixel_basic.hlsl
Texture2D wallTexture : register(t2);
Texture2D grassTexture : register(t3);
Texture2D dirtTexture : register(t4);
Texture2D shadowTexture : register(t5);
Texture2D detailTexture : register(t6);  // NEW - just add this line!
```

**2. Recompile shader** (fxc.exe or Visual Studio build)

**3. Run application:**
- Reflection automatically discovers t6 binding
- Root signature builder adds it to the texture descriptor table
- Descriptor table range automatically expanded: t2-t6 instead of t2-t5
- No C++ changes required!

**4. Load texture in C++ (normal resource creation):**
```cpp
commandList->LoadTexture(L"Detail.dds", m_detailTexture);
```

**Old system would have required:**
- Update root parameter array size
- Update descriptor range count
- Update descriptor table initialization
- Update resource binding code
- Update descriptor allocation count
- Debug why shader and C++ don't match

### Debug Output

When `PrintLayout()` is called, the system outputs the generated root signature structure:

```
=== Root Signature Layout ===
  [0] CBV (b0) - SceneData (Visibility: VERTEX)
  [1] SRV (t0) - ModelSB (Visibility: VERTEX)
  [2] SRV (t1) - PerEntitySB (Visibility: PIXEL)
  [3] Descriptor Table (t2-t5):
      - wallTexture
      - grassTexture
      - dirtTexture
      - shadowTexture
  [4] Root Constants (b0) - LightingPos (8 DWORDs, Visibility: PIXEL)

Static Samplers:
  [s0] Point filter sampler
  [s1] Comparison sampler (LESS_EQUAL)
=============================
```

This output appears in the console/debugger during initialization, allowing verification that the root signature matches expectations.

### Performance Considerations

**Runtime Cost:**
- Reflection happens **once at initialization** when shaders are loaded
- Zero runtime overhead during rendering
- Generated root signatures are identical to manually authored ones
- No performance degradation compared to manual approach

**Build-Time Cost:**
- Reflection parsing adds ~1ms per shader during load
- Negligible compared to overall initialization time
- Can be eliminated in shipping builds by:
  - Pre-generating root signatures at build time
  - Serializing metadata to cache files
  - Using reflection only in debug builds for validation

**Memory Cost:**
- ShaderMetadata stores extracted information (~1-2 KB per shader)
- RootSignatureBuilder temporary data during construction
- Both can be freed after pipeline state is created

### Limitations and Future Enhancements

**Current Limitations:**

1. **Static Samplers Not Reflected**
   - Samplers must still be manually specified
   - DirectX reflection API doesn't provide sampler state details
   - Workaround: Could use shader annotations or metadata files

2. **Pipeline State Not Fully Automatic**
   - Blend state, rasterizer state, depth-stencil state still manual
   - These settings aren't part of shader bytecode
   - Potential solution: JSON sidecar files with additional metadata

3. **Root Signature Wrapper Compatibility**
   - Existing `RootSignature_D12` class expects manual descriptors
   - Currently bypassed by using raw `ID3D12RootSignature`
   - Future: Update wrapper class to accept generated signatures

**Planned Enhancements:**

1. **Metadata Cache System**
   ```cpp
   // Cache reflected metadata to avoid repeated reflection
   if (!LoadMetadataCache("shaders.cache")) {
       ReflectAllShaders();
       SaveMetadataCache("shaders.cache");
   }
   ```

2. **Shader Metadata Files**
   ```json
   // vertex_basic.hlsl.meta
   {
       "rtvFormat": "R8G8B8A8_UNORM",
       "depthFormat": "D32_FLOAT",
       "topology": "TRIANGLE_LIST",
       "cullMode": "BACK",
       "blendState": "OPAQUE"
   }
   ```

3. **Validation and Error Reporting**
   ```cpp
   // Verify at runtime that bound resources match shader expectations
   void ValidateResourceBinding(const char* name) {
       auto it = FindResourceMapping(name);
       if (it == mappings.end()) {
           throw std::runtime_error("Resource not found in shader: " + name);
       }
   }
   ```

4. **Hot Reload Support**
   ```cpp
   // Detect shader file changes and rebuild pipeline state
   if (ShaderFileChanged("vertex_basic.cso")) {
       ReloadShader();
       RebuildPipelineState();
   }
   ```

### Implementation Files

**Core System:**
- [ShaderReflection.h](RandomGen/Rendering/ShaderReflection.h) (82 lines) - Metadata structures and reflection interface
- [ShaderReflection.cpp](RandomGen/Rendering/ShaderReflection.cpp) (192 lines) - DirectX reflection API wrapper
- [RootSignatureBuilder.h](RandomGen/Rendering/RootSignatureBuilder.h) (78 lines) - Root signature generation interface
- [RootSignatureBuilder.cpp](RandomGen/Rendering/RootSignatureBuilder.cpp) (287 lines) - Root signature builder implementation

**Integration:**
- [Renderer_D12.h](RandomGen/Rendering/Renderer_D12.h) - Added shader metadata members (lines 162-169)
- [Renderer_D12.cpp](RandomGen/Rendering/Renderer_D12.cpp) - BuildPipelineState refactored (lines 643-767)
- [Renderer_D12.cpp](RandomGen/Rendering/Renderer_D12.cpp) - BuildShadowPipelineState refactored (lines 769-870)

**Project Files:**
- [RandomGen.vcxproj](RandomGen/RandomGen.vcxproj) - Added new source/header files

**Total Implementation:**
- ~640 lines of new code
- ~80 lines of C++ code eliminated (manual root signature setup)
- Net gain: Infrastructure that scales to any number of shaders

### Conclusion

The self-describing shader pipeline system provides a robust, maintainable solution for DirectX 12 resource binding. By leveraging shader reflection, the system ensures that C++ pipeline state always matches shader requirements, eliminating an entire class of bugs and reducing iteration time for graphics programmers.

The system demonstrates best practices for modern graphics API development:
- **Data-driven design**: Configuration comes from data (shaders) not code
- **Single source of truth**: Shaders are the authoritative source for resource requirements
- **Fail-fast validation**: Mismatches detected immediately at load time
- **Zero-cost abstraction**: No runtime overhead compared to manual approach

This architecture can be extended to other DirectX 12 projects or adapted to Vulkan (via SPIR-V reflection) with minimal changes.

---

— End of `Claude.md`
