#include "Renderer_D12.h"

#include "UploadBuffer_D12.h"
#include "DescriptorAllocator_D12.h"
#include "DescriptorAllocatorPage_D12.h"
#include "DescriptorAllocation_D12.h"
#include "RootSignature_D12.h"
#include "DynamicDescriptorHeap_D12.h"
#include "Texture_D12.h"

#include "Window.h"
#include "MazeGenDefs.h"

// Validate GPU structure sizes to ensure correct memory layout
static_assert(sizeof(SceneData) == 256, "SceneData must be 256 bytes for GPU constant buffer alignment");
static_assert(sizeof(VertexInput) == 48, "VertexInput must be 48 bytes (4 vec3s = 12 bytes each)");
static_assert(sizeof(glm::mat4) == 64, "glm::mat4 must be 64 bytes for GPU compatibility");
static_assert(sizeof(glm::vec3) == 12, "glm::vec3 must be 12 bytes");
static_assert(sizeof(glm::vec4) == 16, "glm::vec4 must be 16 bytes");
static_assert(sizeof(glm::vec2) == 8, "glm::vec2 must be 8 bytes");

bool CheckTearingSupport()
{
	BOOL allowTearing = FALSE;

	// Rather than create the DXGI 1.5 factory interface directly, we create the
	// DXGI 1.4 interface and query for the 1.5 interface. This is to enable the 
	// graphics debugging tools which will not support the 1.5 factory interface 
	// until a future update.
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(
				DXGI_FEATURE_PRESENT_ALLOW_TEARING,
				&allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}
void EnableDebugLayer()
{
#if defined(_DEBUG)
	// Always enable the debug layer before doing anything DX12 related
	// so all possible errors generated while creating DX12 objects
	// are caught by the debug layer.
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}
ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hWnd,
	ComPtr<ID3D12CommandQueue> commandQueue,
	uint32_t width, uint32_t height, uint32_t bufferCount)
{
	ComPtr<IDXGISwapChain4> dxgiSwapChain4;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = { 1, 0 };
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = CheckTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	
	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(
		commandQueue.Get(),
		hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain1));

	// Disable the Alt+Enter fullscreen toggle feature. Switching to fullscreen
	// will be handled manually.
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain4));

	return dxgiSwapChain4;
}
ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory = 0;
		for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
			wchar_t buffer[500];
			swprintf(buffer, 500, L"Adapter %s \n", dxgiAdapterDesc1.Description);
			OutputDebugStringW(buffer);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(),
					D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}
ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));
	// Enable debug messages in debug mode.
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(d3d12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);        // Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,   // I'm really not sure how to avoid this message.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,                         // This warning occurs when using capture frame while graphics debugging.
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,                       // This warning occurs when using capture frame while graphics debugging.
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif

	return d3d12Device2;
}

Renderer_D12::Renderer_D12() {
	EnableDebugLayer();
	m_tearingSupported = CheckTearingSupport();
	m_currentFrame = 0;

	ComPtr<IDXGIAdapter4> dxgiAdapter4 = GetAdapter(Globals::STARTUP_VALS.use_warp);
	int windowWidth   = GAME_WINDOW->GetWidth();
	int windowHeight = GAME_WINDOW->GetHeight();

	m_device = CreateDevice(dxgiAdapter4);
	m_commQueue = std::make_shared<CommandQueue_D12>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto windowHandle = GAME_WINDOW->GetHandle();
	m_swapChain = CreateSwapChain(windowHandle, m_commQueue->GetD3D12CommandQueue(),
		windowWidth, windowHeight, NUM_BACKBUFFER_FRAMES);

	m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	m_viewport = CD3DX12_VIEWPORT(0.f, 0.f, (float)windowWidth, (float)windowHeight);

	m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();	
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData;
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
		if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,
			sizeof(D3D12_FEATURE_DATA_ROOT_SIGNATURE))))
		{
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
		}
		m_highestRootSignatureVersion = featureData.HighestVersion;
	}

	m_initalized = true;
}

void Renderer_D12::PostInit() {

	m_rtvAllocator = std::make_shared<DescriptorAllocator_D12>(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_BACKBUFFER_FRAMES);
	m_rtvs = std::make_shared<DescriptorAllocation_D12>(m_rtvAllocator->Allocate(NUM_BACKBUFFER_FRAMES));
	m_dsvAllocator = std::make_shared<DescriptorAllocator_D12>(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 2);
	m_dsvs = std::make_shared<DescriptorAllocation_D12>(m_dsvAllocator->Allocate(2));
	m_shaderResourceAllocator = std::make_shared<DescriptorAllocator_D12>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_shaderResourceDynHeap = std::make_shared<DynamicDescriptorHeap_D12>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	//ImGUI
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // IF using Docking Branch


		ImGui_ImplWin32_Init(GAME_WINDOW->GetHandle());

		// Setup Platform/Renderer backends
		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device = m_device.Get();
		init_info.CommandQueue = m_commQueue->GetD3D12CommandQueue().Get();
		init_info.NumFramesInFlight = NUM_BACKBUFFER_FRAMES;
		init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.

		// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
		// The example_win32_directx12/main.cpp application include a simple free-list based allocator.


		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = IMGUI_HEAP_SIZE;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_imGUISRVHeap));
		m_imGUISRVHeap->SetName(L"ImGui SRV Descriptor Heap");

		init_info.SrvDescriptorHeap = m_imGUISRVHeap.Get();
		m_imGUIAllocator.Create(init_info.Device, init_info.SrvDescriptorHeap);
		init_info.SrvDescriptorAllocFn = 
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
				RENDERER->m_imGUIAllocator.Alloc(out_cpu_handle, out_gpu_handle);
			};
		init_info.SrvDescriptorFreeFn = 
			[](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { 
			RENDERER->m_imGUIAllocator.Free(cpu_handle, gpu_handle);
			};

		m_imGUIInitalized = ImGui_ImplDX12_Init(&init_info);
	}


	UpdateRenderTargetViews();
}

bool Renderer_D12::GUIInitialized() const {
	return m_imGUIInitalized;
}

void Renderer_D12::ResizeTargets() {

	// Flush the GPU queue to make sure the swap chain's back buffers
	// are not being referenced by an in-flight command list.
	m_commQueue->Flush();
	for (int i = 0; i < NUM_BACKBUFFER_FRAMES; ++i)
	{
		// Any references to the back buffers must be released
		// before the swap chain can be resized.
		m_backbuffers[i].Reset();
		m_perFrameFenceValues[i] = m_perFrameFenceValues[m_currentBufferIdx];
	}
	DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
	ThrowIfFailed(m_swapChain->GetDesc(&swapChainDesc));
	ThrowIfFailed(m_swapChain->ResizeBuffers(NUM_BACKBUFFER_FRAMES, GAME_WINDOW->GetWidth(), GAME_WINDOW->GetHeight(),
		swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

	m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();

	m_viewport = CD3DX12_VIEWPORT(0.0f, 0.0f,
		static_cast<float>(GAME_WINDOW->GetWidth()), static_cast<float>(GAME_WINDOW->GetHeight()));

	UpdateRenderTargetViews();
}

void Renderer_D12::UpdateRenderTargetViews()
{
	for (int i = 0; i < NUM_BACKBUFFER_FRAMES; ++i)
	{
		auto rtvHandle = m_rtvs->GetDescriptorHandle(i);
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_backbuffers[i] = backBuffer;
	}
}

void Renderer_D12::Render() {

	Shadowmap();
	auto commandList = m_commQueue->GetCommandList();

	auto backBuffer = m_backbuffers[m_currentBufferIdx];
	auto rtv = GetCurrentRenderTargetView();
	auto dsv = m_dsvs->GetDescriptorHandle(0);
	auto shadow = m_dsvs->GetDescriptorHandle(1);

	// Clear the render targets.
	{
		commandList->TransitionResource(backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.5f, 0.0f, 0.5f, 1.0f };

		commandList->ClearRTV(rtv, clearColor);
		commandList->ClearDepth(dsv);
	}

	//@ZGTODO Move to CommandList_D12
	{
		auto d3dCommList = commandList->GetGraphicsCommandList();
		d3dCommList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3dCommList->RSSetViewports(1, &m_viewport);
		d3dCommList->RSSetScissorRects(1, &m_scissorRect);

		d3dCommList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		for (const auto& batch : m_materialBatches) {
			if (!batch.material || !batch.material->pso)
				continue;

			d3dCommList->SetPipelineState(batch.material->pso->pipelineState.Get());
			d3dCommList->SetGraphicsRootSignature(batch.material->pso->rootSignature->GetD3D12RootSignature().Get());
			
			
			BindingInfo bi;
			// Bind global SceneData CBV
			if (GetMaterialBindingInfoForResource(*batch.material, SCENE_DATA_BUFFER_NAME, bi))
				d3dCommList->SetGraphicsRootConstantBufferView(bi.rootIndex, m_sceneDataBuffer->m_resource->GetGPUVirtualAddress());

			// Bind global PerEntityData SRV
			if (GetMaterialBindingInfoForResource(*batch.material, PER_ENTITY_DATA_BUFFER_NAME, bi))
				d3dCommList->SetGraphicsRootShaderResourceView(bi.rootIndex, m_perEntityDataBuffer->m_resource->GetGPUVirtualAddress());

			// Bind material-specific textures via dynamic descriptor heap
			m_shaderResourceDynHeap->ParseRootSignature(*batch.material->pso->rootSignature.get());
			for (int i = 0; i < batch.material->textureAttachments.size(); ++i) {
				GetMaterialBindingInfoForResource(*batch.material, batch.material->textureAttachments[i].shaderName, bi);
				m_shaderResourceDynHeap->StageDescriptors(bi.rootIndex, bi.offset, 1, batch.material->textureAttachments[i].texture->GetCPUHandle());
			}
			if (GetMaterialBindingInfoForResource(*batch.material, SHADOW_TEX_NAME, bi))
				m_shaderResourceDynHeap->StageDescriptors(bi.rootIndex, bi.offset, 1, m_shadowTexture->GetCPUHandle());
			m_shaderResourceDynHeap->CommitStagedDescriptorsForDraw(commandList);

			for (const auto& mBatch : batch.meshes) {
				d3dCommList->IASetVertexBuffers(0, 1, &std::get<D3D12_VERTEX_BUFFER_VIEW>(mBatch.mesh->m_vertexBuffer.m_bufferView));
				d3dCommList->IASetIndexBuffer(&std::get<D3D12_INDEX_BUFFER_VIEW>(mBatch.mesh->m_indexBuffer.m_bufferView));

				d3dCommList->DrawIndexedInstanced((UINT)mBatch.mesh->m_indices.size(), mBatch.instanceCount, 0, 0, mBatch.startInstanceOffset);
			}
		}
	}

	// Present
	{

		//ImGUI Draw
		{
			commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, m_imGUISRVHeap.Get());
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetGraphicsCommandList().Get());
		}

		commandList->TransitionResource(backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_perFrameFenceValues[m_currentBufferIdx] = m_commQueue->ExecuteCommandList(commandList);

		UINT syncInterval = Globals::VSYNC_ENABLED ? 1 : 0;
		UINT presentFlags = m_tearingSupported && !Globals::VSYNC_ENABLED ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
		m_commQueue->WaitForFenceValue(m_perFrameFenceValues[m_currentBufferIdx]);
		m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
		m_shaderResourceDynHeap->Reset();

	}
	++m_currentFrame;

	// Periodically release stale descriptors to prevent memory leaks
	// Clean up descriptors that are at least NUM_BACKBUFFER_FRAMES old to ensure GPU is done
	if (m_currentFrame % 60 == 0 && m_currentFrame >= NUM_BACKBUFFER_FRAMES) {
		uint64_t completedFrame = m_currentFrame - NUM_BACKBUFFER_FRAMES;
		m_rtvAllocator->ReleaseStaleDescriptors(completedFrame);
		m_dsvAllocator->ReleaseStaleDescriptors(completedFrame);
		m_shaderResourceAllocator->ReleaseStaleDescriptors(completedFrame);
	}
}

void Renderer_D12::Shadowmap() {
	auto commandList = m_commQueue->GetCommandList();

	auto dsv = m_dsvs->GetDescriptorHandle(1);

	// Clear the render targets.
	{
		commandList->TransitionResource(m_shadowTexture->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ClearDepth(dsv);
	}

	//@ZGTODO Move to CommandList_D12
	{

		auto d3dCommList = commandList->GetGraphicsCommandList();
		auto& shadowMat = m_materialMap[m_shadowmapMatId];
		d3dCommList->SetPipelineState(shadowMat->pso->pipelineState.Get());
		d3dCommList->SetGraphicsRootSignature(shadowMat->pso->rootSignature->GetD3D12RootSignature().Get());
		d3dCommList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//ZGTODO Change this to key based rendering
		std::string lastMesh = "";
		d3dCommList->RSSetViewports(1, &m_viewport);
		d3dCommList->RSSetScissorRects(1, &m_scissorRect);

		d3dCommList->OMSetRenderTargets(0, NULL, FALSE, &dsv);


		// Update the MVP matrix
		BindingInfo bi;
		bool hasBinding = GetMaterialBindingInfoForResource(*shadowMat, SCENE_DATA_BUFFER_NAME, bi);
		d3dCommList->SetGraphicsRootConstantBufferView(bi.rootIndex, m_sceneDataBuffer->m_resource->GetGPUVirtualAddress());
		GetMaterialBindingInfoForResource(*shadowMat, PER_ENTITY_DATA_BUFFER_NAME, bi);
		d3dCommList->SetGraphicsRootShaderResourceView(bi.rootIndex, m_perEntityDataBuffer->m_resource->GetGPUVirtualAddress());

		for (const auto& batch : m_materialBatches) {
			if (!batch.material || !batch.material->pso)
				continue;
			for (const auto& mBatch : batch.meshes) {
				d3dCommList->IASetVertexBuffers(0, 1, &std::get<D3D12_VERTEX_BUFFER_VIEW>(mBatch.mesh->m_vertexBuffer.m_bufferView));
				d3dCommList->IASetIndexBuffer(&std::get<D3D12_INDEX_BUFFER_VIEW>(mBatch.mesh->m_indexBuffer.m_bufferView));

				d3dCommList->DrawIndexedInstanced((UINT)mBatch.mesh->m_indices.size(), mBatch.instanceCount, 0, 0, mBatch.startInstanceOffset);
			}
		}

		commandList->TransitionResource(m_shadowTexture->GetResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	}
	// Present
	{
		m_shadowMapFenceVal = m_commQueue->ExecuteCommandList(commandList);
		m_commQueue->WaitForFenceValue(m_shadowMapFenceVal);
	}
}

Renderer_D12::~Renderer_D12() {

}

void Renderer_D12::Shutdown() {
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	// Make sure the command queue has finished all commands before closing.
	m_commQueue->Flush();
}

bool Renderer_D12::IsInitialized() const {
	return m_initalized;
}

std::shared_ptr<Buffer> Renderer_D12::CreateSRVBuffer(const std::string& name, size_t size, size_t count, void* data, std::shared_ptr<CommandList_D12> cmdList = nullptr) {
	if (cmdList == nullptr)
		cmdList = m_commQueue->GetCommandList();

	auto out = std::make_shared<Buffer>();

	ComPtr<ID3D12Resource> intermediateBuffer;
	cmdList->UpdateBufferResource(m_device,
		&out->m_resource, &intermediateBuffer,
		count, size, data);
	std::wstring stemp = std::wstring(name.begin(), name.end());
	out->m_resource->SetName(stemp.c_str());
	out->m_bufferView = D3D12_VERTEX_BUFFER_VIEW(
		out->m_resource->GetGPUVirtualAddress(),
		(UINT)(size * count),
		(UINT)size
	);
	cmdList->TrackResource(intermediateBuffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = (UINT)count;
	srvDesc.Buffer.StructureByteStride = (UINT)size;
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	out->m_srvAlloc = GetNextSRVAlloc();
	m_device->CreateShaderResourceView(out->m_resource.Get(), &srvDesc, out->m_srvAlloc.cpuHandle);

	return out;
}

std::shared_ptr<Buffer> Renderer_D12::CreateCBVBuffer(const std::string& name, size_t size, void* data,
	void** dataCPUHandle = nullptr, std::shared_ptr<CommandList_D12> cmdList = nullptr) {
	if (cmdList == nullptr)
		cmdList = m_commQueue->GetCommandList();

	auto out = std::make_shared<Buffer>();

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&out->m_resource)));
	std::wstring stemp = std::wstring(name.begin(), name.end());
	out->m_resource->SetName(stemp.c_str());

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = out->m_resource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)size;
	out->m_srvAlloc = GetNextSRVAlloc();
	m_device->CreateConstantBufferView(&cbvDesc, out->m_srvAlloc.cpuHandle);
	out->m_bufferView = D3D12_CONSTANT_BUFFER_VIEW_DESC(
		out->m_resource->GetGPUVirtualAddress(),
		((UINT)(size + 255) & ~255)
		);

	CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(out->m_resource->Map(0, &readRange, dataCPUHandle));
	memcpy(*dataCPUHandle, data, size);

	return out;
}
void Renderer_D12::CreateAndPopulateVertexBuffer(Buffer& buffer, const VertexInput* data, size_t count) {
	auto commandList = m_commQueue->GetCommandList();
	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	commandList->UpdateBufferResource(m_device,
		&buffer.m_resource, &intermediateVertexBuffer,
		count, sizeof(VertexInput), data);
	// Create the vertex buffer view.
	buffer.m_bufferView = D3D12_VERTEX_BUFFER_VIEW(
		buffer.m_resource->GetGPUVirtualAddress(),
		(UINT)(sizeof(VertexInput) * count),
		sizeof(VertexInput));
	commandList->TrackResource(intermediateVertexBuffer);
}

void Renderer_D12::CreateAndPopulateIndexBuffer(Buffer& buffer, const unsigned int *data, size_t count) {
	auto commandList = m_commQueue->GetCommandList();
	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	commandList->UpdateBufferResource(m_device, &buffer.m_resource, &intermediateIndexBuffer,
		count, sizeof(unsigned int), data);

	// Create index buffer view.
	buffer.m_bufferView = D3D12_INDEX_BUFFER_VIEW(
		buffer.m_resource->GetGPUVirtualAddress(),
		(UINT)(sizeof(unsigned int) * count),
		DXGI_FORMAT_R32_UINT);

	commandList->TrackResource(intermediateIndexBuffer);
}

void Renderer_D12::LoadTextures() {
	m_sceneDataBuffer = CreateCBVBuffer("SceneData", sizeof(SceneData), &m_sceneData, reinterpret_cast<void**>(&m_sceneDataPtr));

	std::hash<std::string> hasher;
	auto mat = CreateMaterial("BasicLit", L"vertex_basic", L"pixel_basic", { L"Clay.dds", L"Grass.dds", L"Dirt.dds" });

	int idx = 0;
	//Manual set until i have a UI
	for (const auto& pair : mat->pso->resourceToBindingInfo) {
		if (pair.second.type != D3D_SIT_TEXTURE)
			continue;

		TextureAttachmentInfo tai;
		tai.shaderName = pair.first;

		if (pair.first.compare("wallTexture") == 0)
			tai.texture = MakeOrGetTexture(L"WallTex", L"Clay.dds", m_textureMap);
		else if (pair.first.compare("grassTexture") == 0)
			tai.texture = MakeOrGetTexture(L"GrassTex", L"Grass.dds", m_textureMap);
		else if (pair.first.compare("dirtTexture") == 0)
			tai.texture = MakeOrGetTexture(L"DirtTex", L"Dirt.dds", m_textureMap);
		else
			continue;
		mat->textureAttachments[idx] = tai;
		++idx;
	}

	m_basicLitMatId = hasher("BasicLit");
	CreateMaterial("Shadowmap", L"vertex_shadow", L"pixel_shadow");;
	m_shadowmapMatId = hasher("Shadowmap");
}

void Renderer_D12::UpdateInstanceData(const std::vector<Renderable>& renderables) {

	// Group renderables by material pointer, preserving the shared_ptr from the first renderable in each group
	std::map<Material*, std::pair<std::shared_ptr<Material>, std::vector<const Renderable*>>> materialGroups;
	for (const auto& r : renderables) {
		auto matPtr = r.m_mesh ? r.m_mesh->m_material : nullptr;
		auto& entry = materialGroups[matPtr.get()];
		if (!entry.first)
			entry.first = matPtr;
		entry.second.push_back(&r);
	}

	for (auto& [rawPtr, entry] : materialGroups) {
		auto& [matShared, group] = entry;
		std::sort(group.begin(), group.end(), [](const Renderable* a, const Renderable* b) { return a->m_mesh->m_name < b->m_mesh->m_name; });
	}

	// Flatten grouped renderables into a single PerEntityData buffer and build batch list
	std::vector<PerEntityData> peds;
	peds.reserve(renderables.size());
	m_materialBatches.clear();

	float maxZ = 0.0f;
	for (auto& [rawPtr, entry] : materialGroups) {
		auto& [matShared, group] = entry;
		if (!matShared || group.empty())
			continue;

		MaterialBatch batch;
		batch.material = matShared;

		MeshBatch mBatch{};
		mBatch.mesh = group.front()->m_mesh;
		mBatch.startInstanceOffset = peds.size();
		mBatch.instanceCount = 0;

		for (const Renderable* r : group) {
			if (r->m_mesh != mBatch.mesh) {
				batch.meshes.push_back(mBatch);     // flush previous mesh
				mBatch = {};
				mBatch.mesh = r->m_mesh;
				mBatch.startInstanceOffset = peds.size();
				mBatch.instanceCount = 0;
			}

			glm::mat4 M = glm::translate(glm::mat4(1.0f), r->m_position)
				* glm::mat4_cast(r->m_orientation)
				* glm::scale(glm::mat4(1.0f), r->m_scale);
			peds.push_back({ M, r->m_entityData });

			if (r->m_position.z > maxZ)
				maxZ = r->m_position.z;

			++mBatch.instanceCount;
		}

		batch.meshes.push_back(mBatch);             // flush final mesh
		m_materialBatches.push_back(std::move(batch));
	}

	m_worldWidth = (int)(maxZ + 2);
	m_perEntityDataBuffer = CreateSRVBuffer("PerEntityBuffer", sizeof(PerEntityData), peds.size(), peds.data());

	auto fenceValue = m_commQueue->ExecuteActiveCommandList();
	m_commQueue->WaitForFenceValue(fenceValue);
}

std::shared_ptr<Texture_D12> Renderer_D12::MakeOrGetTexture(const std::wstring& name, const std::wstring& filepath, 
	std::map<size_t, std::shared_ptr<Texture_D12>>& resourceMap, std::shared_ptr<CommandList_D12> cmdList) {
	std::hash<std::wstring> whasher;
	size_t id = whasher(name);
	if (resourceMap.contains(id)) {
		return resourceMap[id];
	}

	std::shared_ptr<Texture_D12> tex = std::make_shared<Texture_D12>();
	tex->SetCPUAllocation(GetNextSRVAlloc());
	if (filepath.size() > 0) {
		if (cmdList == nullptr)
			cmdList = m_commQueue->GetCommandList();
		cmdList->LoadTexture(name, filepath, tex);

		resourceMap[id] = tex;
	}

	return tex;
}

SRVAllocation Renderer_D12::GetNextSRVAlloc() {
	SRVAllocation out;
	int pageIdx = -1;
	for (int j = 0; j < m_srvAllocPages.size(); ++j) {
		if (!m_srvAllocPages[j].IsFull())
			pageIdx = j;
	}
	if (pageIdx == -1) {
		SRVAllocationPage srvAP(std::move(m_shaderResourceAllocator->Allocate(128)));
		out = srvAP.GetNextHandle();
		m_srvAllocPages.emplace_back(std::move(srvAP));

	}
	else {
		out = m_srvAllocPages[pageIdx].GetNextHandle();
	}

	return out;
}


std::shared_ptr<Mesh> Renderer_D12::BuildMesh(std::string name, std::vector<VertexInput> vertices, std::vector<unsigned int> indices, std::string material) {
	if (vertices.empty() || indices.empty())
		return nullptr;

	std::shared_ptr<Mesh> out = std::make_shared<Mesh>();
	out->m_vertices = vertices;
	out->m_indices = indices;

	CreateAndPopulateVertexBuffer(out->m_vertexBuffer, vertices.data(), vertices.size());
	CreateAndPopulateIndexBuffer(out->m_indexBuffer, indices.data(), indices.size());

	out->m_material = GetMaterial(material);
	return out;
}

std::shared_ptr<Material> Renderer_D12::CreateMaterial(const std::string name, const std::wstring& vertexShaderName, const std::wstring& pixelShaderName, const std::vector<std::wstring>& textures) {
	std::hash<std::string> hasher;
	size_t matId = hasher(name);

	if (m_materialMap.contains(matId))
		return m_materialMap[matId];

	auto mat = std::make_shared<Material>();
	mat->name = name;

	mat->vertexShader = vertexShaderName;
	mat->pixelShader = pixelShaderName;

	mat->pso = BuildPipelineState(vertexShaderName, pixelShaderName);
	mat->textureAttachments.resize(textures.size());

	auto cmdList = m_commQueue->GetCommandList();
	m_materialMap[matId] = mat;
	return mat;
}

std::shared_ptr<Material> Renderer_D12::GetMaterial(const std::string& name) const {
	std::hash<std::string> hasher;
	size_t matId = hasher(name);
	auto it = m_materialMap.find(matId);
	if (it != m_materialMap.end())
		return it->second;
	return nullptr;
}

PipelineStateObject* Renderer_D12::BuildPipelineState(const std::wstring& vertexShaderName, const std::wstring& pixelShaderName) {

	std::hash<std::wstring> hasher;
	size_t psoId = hasher(vertexShaderName + pixelShaderName);
	if (m_psoMap.contains(psoId))
		return &m_psoMap[psoId];

	PipelineStateObject entry;

	// Reflect shaders to extract metadata
	Rendering::ShaderReflector reflector;
	Rendering::ShaderMetadata vertexMetadata;
	Rendering::ShaderMetadata pixelMetadata;
	if (!reflector.ReflectShader(vertexShaderName, vertexMetadata)) {
		throw std::runtime_error("Failed to reflect shadow vertex shader");
	}
	if (!reflector.ReflectShader(pixelShaderName, pixelMetadata)) {
		throw std::runtime_error("Failed to reflect shadow pixel shader");
	}

	// Build input layout automatically from vertex shader
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
	for (const auto& element : vertexMetadata.inputElements) {
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

	Rendering::RootSignatureBuilder rootSigBuilder;
	rootSigBuilder.AddShaderStage(vertexMetadata);
	rootSigBuilder.AddShaderStage(pixelMetadata);

	for (int i = 0; i < pixelMetadata.staticSamplers.size(); ++i) {
		rootSigBuilder.AddStaticSampler(pixelMetadata.staticSamplers[i]);
	}

	// Build the root signature description - automatically matches shader requirements!
	D3D12_ROOT_SIGNATURE_DESC1 rootSigDesc = {};
	if (!rootSigBuilder.Build(rootSigDesc)) {
		throw std::runtime_error("Failed to build shadow root signature from shader metadata");
	}

	auto mappings = rootSigBuilder.GetResourceMappings();
	uint32_t offset = 0;
	uint32_t lastRootIdx = 0;
	for (int i = 0; i < mappings.size(); ++i) {
		uint32_t rootIdx = mappings[i].rootParameterIndex;
		if (rootIdx == lastRootIdx) {
			++offset;
		}
		else {
			offset = 0;
		}
		entry.resourceToBindingInfo[mappings[i].resourceName] = { rootIdx, offset, mappings[i].type};
		lastRootIdx = rootIdx;
	}

	// Print the generated layout for debugging
	rootSigBuilder.PrintLayout();

	// Create RootSignature_D12 from the auto-generated description
	entry.rootSignature = std::make_shared<RootSignature_D12>(rootSigDesc);

	// ========== SHADOW PIPELINE STATE WITH AUTO-GENERATED COMPONENTS ==========
	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
		CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
	} pipelineStateStream;

	// Use the automatically generated root signature
	pipelineStateStream.pRootSignature = entry.rootSignature->GetD3D12RootSignature().Get();
	// Use the automatically generated input layout
	pipelineStateStream.InputLayout = { inputLayout.data(), static_cast<UINT>(inputLayout.size()) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexMetadata.shaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelMetadata.shaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = (UINT)pixelMetadata.renderDefs.renderTargets.size();
	for (unsigned int i = 0; i < rtvFormats.NumRenderTargets; ++i) {
		rtvFormats.RTFormats[i] = pixelMetadata.renderDefs.renderTargets[i];
	}
	pipelineStateStream.RTVFormats = rtvFormats;

	CD3DX12_RASTERIZER_DESC raster(D3D12_DEFAULT);
	raster.CullMode = pixelMetadata.renderDefs.cullMode;
	pipelineStateStream.Rasterizer = raster;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&entry.pipelineState)));

	m_psoMap[psoId] = entry;

	return &m_psoMap[psoId];
}

// Resize the depth buffer to match the size of the client area.
void Renderer_D12::ResizeDepthBuffer(int width, int height) {

	m_commQueue->Flush();
	width = std::max(1, width);
	height = std::max(1, height);

	// Resize screen dependent resources.
	// Create a depth buffer.
	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	optimizedClearValue.DepthStencil = { 1.0f, 0 };

	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
			1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&m_depthBuffer)
	));

	// Update the depth-stencil view.
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	m_device->CreateDepthStencilView(m_depthBuffer.Get(), &dsv,
		m_dsvs->GetDescriptorHandle(0));

	ComPtr<ID3D12Resource> resource;
	//Shadowmap
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height,
			1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&optimizedClearValue,
		IID_PPV_ARGS(&resource)
	));

	auto descStep = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateDepthStencilView(resource.Get(), &dsv,
		m_dsvs->GetDescriptorHandle(1));
	m_shadowTexture = MakeOrGetTexture(L"SunShadow", L"", m_textureMap);
	m_shadowTexture->SetResource(resource);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(m_shadowTexture->GetResource().Get(), &srvDesc,
		m_shadowTexture->GetCPUHandle());

	m_sceneData.invShadowTexSize = glm::vec2(1.f / width, 1.f / height);
}

void Renderer_D12::UpdateMVP(float fov, glm::vec3 camPos, glm::vec3 camFwd, glm::vec3 camRight, glm::vec3 camUp, glm::vec4 sunPos) {
	// Update the view matrix.
	m_sceneData.camPos = glm::vec4(camPos, 1.0f);
	glm::vec3 upDirection = glm::cross(camFwd, camRight);
	auto view = glm::lookAtLH(camPos, camPos + camFwd, upDirection);

	// Update the projection matrix.
	float aspectRatio = GAME_WINDOW->GetWidth() / static_cast<float>(GAME_WINDOW->GetHeight());
	auto proj = glm::perspectiveFovLH(glm::radians(fov), (float)GAME_WINDOW->GetWidth(), (float)GAME_WINDOW->GetHeight(), 0.1f, 100.0f);

	m_sceneData.camVP = proj * view;

	//Sun
	glm::vec3 lookAtPos = glm::vec3(m_worldWidth / 2.f, 0, m_worldWidth / 2.f);
	glm::vec3 sunDir = glm::normalize(glm::vec3(lookAtPos - glm::vec3(sunPos)));
	const glm::vec3 rightDirection = glm::cross(glm::vec3(0.f, 1.f, 0.f), sunDir);
	upDirection = glm::cross(sunDir, rightDirection);
	view = glm::lookAtLH(glm::vec3(sunPos), glm::vec3(sunPos) + sunDir, upDirection);
	// Update the projection matrix.
	proj = glm::orthoLH(-m_worldWidth * 0.75f, m_worldWidth * 0.75f, -m_worldWidth * 0.75f, m_worldWidth * 0.75f, 0.1f, 100.0f);
	m_sceneData.sunVP = proj * view;

	m_sceneData.sunPos = sunPos;
	memcpy(m_sceneDataPtr, &m_sceneData, sizeof(SceneData));
}


ComPtr<ID3D12Device2> Renderer_D12::GetDevice() const {
	return m_device;
}

uint64_t  Renderer_D12::GetCurrentFrameCount() const {
	return m_currentFrame;
}

uint32_t Renderer_D12::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const {
	return m_device->GetDescriptorHandleIncrementSize(type);
}

D3D_ROOT_SIGNATURE_VERSION Renderer_D12::GetHighestRootSigVer() const {
	return m_highestRootSignatureVersion;
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer_D12::GetCurrentRenderTargetView() const
{
	return m_rtvs->GetDescriptorHandle(m_currentBufferIdx);
}