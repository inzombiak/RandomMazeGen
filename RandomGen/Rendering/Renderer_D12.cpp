#include "Renderer_D12.h"

#include "UploadBuffer_D12.h"
#include "DescriptorAllocator_D12.h"
#include "DescriptorAllocation_D12.h"
#include "RootSignature_D12.h"
#include "DynamicDescriptorHeap_D12.h"
#include "Texture_D12.h"

#include "Window.h"
#include "../Tile.h"

using namespace DirectX;
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

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
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
	// It is recommended to always allow tearing if tearing support is available.
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
	m_shaderResources = std::make_shared<DescriptorAllocation_D12>(m_shaderResourceAllocator->Allocate(7));
	m_shaderResourceDynHeap = std::make_shared<DynamicDescriptorHeap_D12>(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	UpdateRenderTargetViews();
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
	auto commandList = m_commQueue->GetCommandList();

	auto backBuffer = m_backbuffers[m_currentBufferIdx];
	auto rtv = GetCurrentRenderTargetView();
	auto dsv = m_dsvs->GetDescriptorHandle(0);
	auto shadow = m_dsvs->GetDescriptorHandle(1);

	// Clear the render targets.
	{
		commandList->TransitionResource(backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		commandList->TransitionResource(m_shadowTexture->GetResource(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);


		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		commandList->ClearRTV(rtv, clearColor);
		commandList->ClearDepth(dsv);
	}

	//@ZGTODO Move to CommandList_D12
	{
		auto d3dCommList = commandList->GetGraphicsCommandList();
		d3dCommList->SetPipelineState(m_pipelineState.Get());
		d3dCommList->SetGraphicsRootSignature(m_rootSignature->GetD3D12RootSignature().Get());
		d3dCommList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3dCommList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		d3dCommList->IASetIndexBuffer(&m_indexBufferView);

		d3dCommList->RSSetViewports(1, &m_viewport);
		d3dCommList->RSSetScissorRects(1, &m_scissorRect);
		
		d3dCommList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

		// Update the MVP matrixb
		d3dCommList->SetGraphicsRootConstantBufferView(0, m_vpBufferView.BufferLocation);
		d3dCommList->SetGraphicsRootShaderResourceView(1, m_modelBufferView.BufferLocation);
		d3dCommList->SetGraphicsRootShaderResourceView(2, m_entityDataBufferView.BufferLocation);
		m_shaderResourceDynHeap->ParseRootSignature(*m_rootSignature.get());
		m_shaderResourceDynHeap->StageDescriptors(3, 0, 4, m_shaderResources->GetDescriptorHandle(3));
		m_shaderResourceDynHeap->CommitStagedDescriptorsForDraw(commandList);
		d3dCommList->SetGraphicsRoot32BitConstants(4, sizeof(LightingData), &m_lightingData, 0);
		d3dCommList->DrawIndexedInstanced((UINT)m_indexCount, m_numInstances, 0, 0, 0);
	}
	// Present
	{
		commandList->TransitionResource(backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		commandList->TransitionResource(m_shadowTexture->GetResource(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		m_perFrameFenceValues[m_currentBufferIdx] = m_commQueue->ExecuteCommandList(commandList);

		UINT syncInterval = Globals::VSYNC_ENABLED ? 1 : 0;
		UINT presentFlags = m_tearingSupported && !Globals::VSYNC_ENABLED ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
		m_commQueue->WaitForFenceValue(m_perFrameFenceValues[m_currentBufferIdx]);
		m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();
		m_shaderResourceDynHeap->Reset();

	}
	++m_currentFrame;
}

void Renderer_D12::Shadowmap(XMVECTOR sunPos) {
	auto commandList = m_commQueue->GetCommandList();

	auto dsv = m_dsvs->GetDescriptorHandle(1);

	// Clear the render targets.
	{
		commandList->ClearDepth(dsv);
	}

	//@ZGTODO Move to CommandList_D12
	{
		auto d3dCommList = commandList->GetGraphicsCommandList();
		d3dCommList->SetPipelineState(m_shadowPipelineState.Get());
		d3dCommList->SetGraphicsRootSignature(m_shadowRootSignature->GetD3D12RootSignature().Get());
		d3dCommList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		d3dCommList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		d3dCommList->IASetIndexBuffer(&m_indexBufferView);

		d3dCommList->RSSetViewports(1, &m_viewport);
		d3dCommList->RSSetScissorRects(1, &m_scissorRect);

		d3dCommList->OMSetRenderTargets(0, NULL, FALSE, &dsv);

		// Update the MVP matrix
		XMStoreFloat4(&m_lightingData.sunPos, sunPos);
		XMVECTOR lookAtPos = XMVectorSet(m_worldWidth/2.f, 0, m_worldWidth / 2.f, 1.f);
		XMVECTOR sunDir = XMVector4Normalize(lookAtPos - sunPos);
		const XMVECTOR rightDirection = XMVector3Cross(XMVectorSet(0.f, 1.f, 0.f, 0.f), sunDir);
;		const XMVECTOR upDirection = XMVector3Cross(sunDir, rightDirection);
		auto viewMat = XMMatrixLookAtLH(sunPos, sunPos + sunDir, upDirection);

		// Update the projection matrix.
		auto projMat = XMMatrixOrthographicLH(m_worldWidth * 1.5, m_worldWidth * 1.5, 0.1f, 100.0f);
		m_sceneData.sunVP = XMMatrixMultiply(viewMat, projMat);
		d3dCommList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &m_sceneData.sunVP, 0);
		d3dCommList->SetGraphicsRootShaderResourceView(1, m_modelBufferView.BufferLocation);
		d3dCommList->DrawIndexedInstanced((UINT)m_indexCount, m_numInstances, 0, 0, 0);
	}
	// Present
	{
		m_shadowMapFenceVal = m_commQueue->ExecuteCommandList(commandList);
		m_commQueue->WaitForFenceValue(m_shadowMapFenceVal);
	}
}


void Renderer_D12::Shutdown() {
	// Make sure the command queue has finished all commands before closing.
	m_commQueue->Flush();
}

bool Renderer_D12::IsInitialized() const {
	return m_initalized;
}

void Renderer_D12::PopulateVertexBuffer(const VertexInput* data, size_t count) {
	auto commandList = m_commQueue->GetCommandList();
	// Upload vertex buffer data.
	ComPtr<ID3D12Resource> intermediateVertexBuffer;
	commandList->UpdateBufferResource(m_device,
		&m_vertexBuffer, &intermediateVertexBuffer,
		count, sizeof(VertexInput), data);
	// Create the vertex buffer view.
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = (UINT)(sizeof(VertexInput) * count);
	m_vertexBufferView.StrideInBytes = sizeof(VertexInput);
	commandList->TrackResource(intermediateVertexBuffer);
}

void Renderer_D12::PopulateIndexBuffer(const WORD *data, size_t count) {
	auto commandList = m_commQueue->GetCommandList();
	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	commandList->UpdateBufferResource(m_device, &m_indexBuffer, &intermediateIndexBuffer,
		count, sizeof(WORD), data);

	// Create index buffer view.
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = (UINT)(sizeof(WORD) * count);

	m_indexCount = count;
	commandList->TrackResource(intermediateIndexBuffer);
}

void Renderer_D12::LoadTextures() {

	auto commandList = m_commQueue->GetCommandList();
	
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(sizeof(m_sceneData)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vpBuffer)));

	// Describe and create a constant buffer view.
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_vpBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = sizeof(m_sceneData);
	m_vpCPUHandle = m_shaderResources->GetDescriptorHandle(1);
	m_device->CreateConstantBufferView(&cbvDesc, m_vpCPUHandle);

	m_vpBufferView.BufferLocation = m_vpBuffer->GetGPUVirtualAddress();
	m_vpBufferView.SizeInBytes = (UINT)(sizeof(XMMATRIX) * m_numInstances);
	m_vpBufferView.StrideInBytes = sizeof(XMMATRIX);

	CD3DX12_RANGE readRange(0, 0); 
	ThrowIfFailed(m_vpBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_sceneDataBegin)));
	memcpy(m_sceneDataBegin, &m_sceneData, sizeof(m_sceneData));
	m_wallTexture = std::make_shared<Texture_D12>();
	m_wallTexture->SetCPUHandle(m_shaderResources->GetDescriptorHandle(3));
	commandList->LoadTexture(L"Clay.dds", m_wallTexture);

	m_grassTexture = std::make_shared<Texture_D12>();
	m_grassTexture->SetCPUHandle(m_shaderResources->GetDescriptorHandle(4));
	commandList->LoadTexture(L"Grass.dds", m_grassTexture);

	m_dirtTexture = std::make_shared<Texture_D12>();
	m_dirtTexture->SetCPUHandle(m_shaderResources->GetDescriptorHandle(5));
	commandList->LoadTexture(L"Dirt.dds", m_dirtTexture);



}

void Renderer_D12::CreateSRVForBoxes(const std::vector<std::vector<Tile>>& tiles, double t) {

	m_numInstances = 0;
	std::vector<XMMATRIX>		mvpMatrices;
	std::vector<PerEntityData>  peds;

	int x = 0;
	int y = 0;
	int z = 0;
	for (int i = 0; i < tiles.size(); ++i) {
		x = 0;
		for (int j = 0; j < tiles[i].size(); ++j) {
			y = 0;
			int height = 1;
			if (tiles[i][j].GetType() == GameDefs::TileType::Empty) {
				x += 2;
				continue;
			}

			for (int h = 0; h < height; ++h) {
				XMMATRIX modelMat = XMMatrixTranslation((float)x, (float)y, (float)z);
				mvpMatrices.push_back(modelMat);
				peds.push_back({ 0 });
				y += 2;
			}

			for (int p = 0; p < 4; ++p) {
				if (!tiles[i][j].HasDirection(GameDefs::DIRECTIONS[p])) {
					float wallX = (float)x;
					float wallZ = (float)z;
					float scaleX = 1;
					float scaleZ = 1;

					auto delta = GameDefs::DIRECTION_CHANGES[p];
					wallX += delta.second;
					wallZ += delta.first;

					scaleX -= abs(delta.second) * 0.9f;
					scaleZ -= abs(delta.first) * 0.9f;

					XMMATRIX modelMat = XMMatrixTranslation(wallX, (float)y, wallZ);
					modelMat = XMMatrixScaling(scaleX, 1, scaleZ) * modelMat;
					mvpMatrices.push_back(modelMat);
					peds.push_back({ 1 });
				}
			}

			x += 2;
		}
		z += 2;
	}
	m_worldWidth = z;
	auto descStep = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	auto commandList = m_commQueue->GetCommandList();
	m_numInstances = mvpMatrices.size();
	// Create a buffer and upload the MVP matrices to the GPU
{
	D3D12_SUBRESOURCE_DATA mvpData = {};
	mvpData.pData = mvpMatrices.data();
	mvpData.RowPitch = sizeof(XMMATRIX) * m_numInstances;
	mvpData.SlicePitch = mvpData.RowPitch;

	ComPtr<ID3D12Resource> intermediateBuffer;
	commandList->UpdateBufferResource(m_device,
		&m_modelBuffer, &intermediateBuffer,
		m_numInstances, sizeof(XMMATRIX), mvpMatrices.data());

	m_modelBufferView.BufferLocation = m_modelBuffer->GetGPUVirtualAddress();
	m_modelBufferView.SizeInBytes = (UINT)(sizeof(XMMATRIX) * m_numInstances);
	m_modelBufferView.StrideInBytes = sizeof(XMMATRIX);
	commandList->TrackResource(intermediateBuffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_numInstances;
	srvDesc.Buffer.StructureByteStride = sizeof(XMMATRIX);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_modelCPUHandle = m_shaderResources->GetDescriptorHandle(0);
	m_device->CreateShaderResourceView(m_modelBuffer.Get(), &srvDesc, m_modelCPUHandle);
}

{
	D3D12_SUBRESOURCE_DATA pedData = {};
	pedData.pData = peds.data();
	pedData.RowPitch = sizeof(PerEntityData) * m_numInstances;
	pedData.SlicePitch = pedData.RowPitch;

	ComPtr<ID3D12Resource> intermediateBuffer;
	commandList->UpdateBufferResource(m_device,
		&m_entityDataBuffer, &intermediateBuffer,
		m_numInstances, sizeof(PerEntityData), peds.data());

	m_entityDataBufferView.BufferLocation = m_entityDataBuffer->GetGPUVirtualAddress();
	m_entityDataBufferView.SizeInBytes = (UINT)(sizeof(PerEntityData) * m_numInstances);
	m_entityDataBufferView.StrideInBytes = sizeof(PerEntityData);
	commandList->TrackResource(intermediateBuffer);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_numInstances;
	srvDesc.Buffer.StructureByteStride = sizeof(PerEntityData);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_entityDataCPUHandle = m_shaderResources->GetDescriptorHandle(2);
	m_device->CreateShaderResourceView(m_entityDataBuffer.Get(), &srvDesc, m_entityDataCPUHandle);
}
	auto fenceValue = m_commQueue->ExecuteActiveCommandList();
	m_commQueue->WaitForFenceValue(fenceValue);
}


void Renderer_D12::BuildPipelineState(const std::wstring& vertexShaderName, const std::wstring& pixelShaderName) {
	// Load the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(vertexShaderName.data(), &vertexShaderBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(pixelShaderName.data(), &pixelShaderBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// For MVP
	CD3DX12_DESCRIPTOR_RANGE1 ranges[1];
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	CD3DX12_ROOT_PARAMETER1 rootParameters[5];
	rootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[2].InitAsShaderResourceView(1, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_PIXEL);
	CD3DX12_DESCRIPTOR_RANGE1 texture1Range(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 2);
	rootParameters[3].InitAsDescriptorTable(1, &texture1Range, D3D12_SHADER_VISIBILITY_PIXEL);
	rootParameters[4].InitAsConstants(sizeof(LightingData), 0, 0, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC samplers[2];
	samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[0].MipLODBias = 0;
	samplers[0].MaxAnisotropy = 0;
	samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[0].MinLOD = 0.0f;
	samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[0].ShaderRegister = 0;
	samplers[0].RegisterSpace = 0;
	samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	samplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplers[1].MipLODBias = 0;
	samplers[1].MaxAnisotropy = 0;
	samplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
	samplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	samplers[1].MinLOD = 0.0f;
	samplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	samplers[1].ShaderRegister = 1;
	samplers[1].RegisterSpace = 0;
	samplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 2, samplers, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	m_rootSignature = std::make_shared<RootSignature_D12>(rootSignatureDescription.Desc_1_1);

	struct PipelineStateStream
	{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopologyType;
		CD3DX12_PIPELINE_STATE_STREAM_VS VS;
		CD3DX12_PIPELINE_STATE_STREAM_PS PS;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DSVFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RTVFormats;
	} pipelineStateStream;

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = m_rootSignature->GetD3D12RootSignature().Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.PS = CD3DX12_SHADER_BYTECODE(pixelShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;
	
	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
	sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_pipelineState)));


}

void Renderer_D12::BuildShadowPipelineState(const std::wstring& vertexShaderName, const std::wstring& pixelShaderName) {
	// Load the vertex shader.
	ComPtr<ID3DBlob> vertexShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(vertexShaderName.data(), &vertexShaderBlob));

	// Load the pixel shader.
	ComPtr<ID3DBlob> pixelShaderBlob;
	ThrowIfFailed(D3DReadFileToBlob(pixelShaderName.data(), &pixelShaderBlob));

	// Create the vertex input layout
	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// Create a root signature.
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if (FAILED(m_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData))))
	{
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
	}

	// Allow input layout and deny unnecessary access to certain pipeline stages.
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// For MVP
	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	D3D12_STATIC_SAMPLER_DESC sampler = {};
	sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	sampler.MipLODBias = 0;
	sampler.MaxAnisotropy = 0;
	sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	sampler.MinLOD = 0.0f;
	sampler.MaxLOD = D3D12_FLOAT32_MAX;
	sampler.ShaderRegister = 0;
	sampler.RegisterSpace = 0;
	sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 1, &sampler, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));

	m_shadowRootSignature = std::make_shared<RootSignature_D12>(rootSignatureDescription.Desc_1_1);

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

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 0;

	pipelineStateStream.pRootSignature = m_shadowRootSignature->GetD3D12RootSignature().Get();
	pipelineStateStream.InputLayout = { inputLayout, _countof(inputLayout) };
	pipelineStateStream.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.VS = CD3DX12_SHADER_BYTECODE(vertexShaderBlob.Get());
	pipelineStateStream.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.RTVFormats = rtvFormats;
	CD3DX12_RASTERIZER_DESC raster(D3D12_DEFAULT);
	raster.CullMode = D3D12_CULL_MODE_BACK;
	pipelineStateStream.Rasterizer = raster;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
	sizeof(PipelineStateStream), &pipelineStateStream
	};
	ThrowIfFailed(m_device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_shadowPipelineState)));
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
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&optimizedClearValue,
		IID_PPV_ARGS(&resource)
	));

	auto descStep = GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_device->CreateDepthStencilView(resource.Get(), &dsv,
		m_dsvs->GetDescriptorHandle(1));
	m_shadowTexture = std::make_shared<Texture_D12>();
	m_shadowTexture->SetResource(resource);
	m_shadowTexture->SetCPUHandle(m_shaderResources->GetDescriptorHandle(6));
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	m_device->CreateShaderResourceView(m_shadowTexture->GetResource().Get(), &srvDesc,
		m_shadowTexture->GetCPUHandle());

	m_lightingData.invShadowTexSize = XMFLOAT2(1.f / width, 1.f / height);
}

void Renderer_D12::UpdateMVP(float fov, DirectX::XMVECTOR camPos, DirectX::XMVECTOR camFwd, DirectX::XMVECTOR camRight, DirectX::XMVECTOR camUp) {
	// Update the view matrix.
	XMStoreFloat4(&m_lightingData.camPos, camPos);
	const XMVECTOR upDirection = XMVector3Cross(camFwd, camRight);
	auto view = XMMatrixLookAtLH(camPos, camPos + camFwd, upDirection);

	// Update the projection matrix.
	float aspectRatio = GAME_WINDOW->GetWidth() / static_cast<float>(GAME_WINDOW->GetHeight());
	auto proj = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov), aspectRatio, 0.1f, 100.0f);

	m_sceneData.camVP = XMMatrixMultiply(view, proj);

	memcpy(m_sceneDataBegin, &m_sceneData, sizeof(m_sceneData));
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