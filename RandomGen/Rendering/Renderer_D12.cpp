#include "Renderer_D12.h"

#include "UploadBuffer_D12.h"

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

	m_uploadBuffer = std::make_shared<UploadBuffer_D12>();

	int windowWidth   = GAME_WINDOW->GetWidth();
	int windowHeight = GAME_WINDOW->GetHeight();

	m_device = CreateDevice(dxgiAdapter4);
	m_commQueue = std::make_shared<CommandQueue>(m_device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	auto windowHandle = GAME_WINDOW->GetHandle();
	m_swapChain = CreateSwapChain(windowHandle, m_commQueue->GetD3D12CommandQueue(),
		windowWidth, windowHeight, NUM_BACKBUFFER_FRAMES);

	m_scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);
	m_viewport = CD3DX12_VIEWPORT(0.f, 0.f, (float)windowWidth, (float)windowHeight);

	m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();

	m_rtvHeap = CreateDescriptorHeap(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, NUM_BACKBUFFER_FRAMES);
	m_rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC cbvSrvUavHeapDesc = {};
	cbvSrvUavHeapDesc.NumDescriptors = 1;
	cbvSrvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvSrvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvSrvUavHeapDesc, IID_PPV_ARGS(&m_cbvSrvUavHeap)));

	// Create the descriptor heap for the depth-stencil view.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(m_device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)));

	UpdateRenderTargetViews();


	m_initalized = true;
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
	auto rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < NUM_BACKBUFFER_FRAMES; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);

		m_backbuffers[i] = backBuffer;

		rtvHandle.Offset(rtvDescriptorSize);
	}
}

void Renderer_D12::Render() {
	auto commandList = m_commQueue->GetCommandList();

	auto backBuffer = m_backbuffers[m_currentBufferIdx];
	auto rtv = GetCurrentRenderTargetView();
	auto dsv = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

	// Clear the render targets.
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		FLOAT clearColor[] = { 0.4f, 0.6f, 0.9f, 1.0f };

		ClearRTV(commandList, rtv, clearColor);
		ClearDepth(commandList, dsv);
	}
	commandList->SetPipelineState(m_pipelineState.Get());
	commandList->SetGraphicsRootSignature(m_rootSignature.Get());
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	commandList->IASetIndexBuffer(&m_indexBufferView);
	commandList->RSSetViewports(1, &m_viewport);
	commandList->RSSetScissorRects(1, &m_scissorRect);

	commandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	// Update the MVP matrix
	XMMATRIX vpMatrix = XMMatrixMultiply(m_viewMatrix, m_projectionMatrix);
	commandList->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &vpMatrix, 0);
	commandList->SetGraphicsRootShaderResourceView(1, m_modelBufferView.BufferLocation);
	commandList->DrawIndexedInstanced((UINT)m_indexCount, m_numInstances, 0, 0, 0);

	// Present
	{
		TransitionResource(commandList, backBuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

		m_perFrameFenceValues[m_currentBufferIdx] = m_commQueue->ExecuteCommandList(commandList);

		UINT syncInterval = Globals::VSYNC_ENABLED ? 1 : 0;
		UINT presentFlags = m_tearingSupported && !Globals::VSYNC_ENABLED ? DXGI_PRESENT_ALLOW_TEARING : 0;
		ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
		m_currentBufferIdx = m_swapChain->GetCurrentBackBufferIndex();

		m_commQueue->WaitForFenceValue(m_perFrameFenceValues[m_currentBufferIdx]);
	}
	++m_currentFrame;
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
	UpdateBufferResource(commandList.Get(),
		&m_vertexBuffer, &intermediateVertexBuffer,
		count, sizeof(VertexInput), data);
	// Create the vertex buffer view.
	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.SizeInBytes = (UINT)(sizeof(VertexInput) * count);
	m_vertexBufferView.StrideInBytes = sizeof(VertexInput);
	auto fenceValue = m_commQueue->ExecuteCommandList(commandList);
	m_commQueue->WaitForFenceValue(fenceValue);
}

void Renderer_D12::PopulateIndexBuffer(const WORD *data, size_t count) {
	auto commandList = m_commQueue->GetCommandList();
	// Upload index buffer data.
	ComPtr<ID3D12Resource> intermediateIndexBuffer;
	UpdateBufferResource(commandList.Get(),
		&m_indexBuffer, &intermediateIndexBuffer,
		count, sizeof(WORD), data);

	// Create index buffer view.
	m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
	m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
	m_indexBufferView.SizeInBytes = (UINT)(sizeof(WORD) * count);

	m_indexCount = count;
	auto fenceValue = m_commQueue->ExecuteCommandList(commandList);
	m_commQueue->WaitForFenceValue(fenceValue);
}

void Renderer_D12::CreateSRVForBoxes(const std::vector<std::vector<Tile>>& tiles, double t) {
	m_numInstances = 0;
	std::vector<XMMATRIX> mvpMatrices;
	std::vector<XMVECTOR> colors;

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
				XMMATRIX modelMat = XMMatrixTranslation(x, y, z);
				mvpMatrices.push_back(modelMat);
				y += 2;
			}

			for (int p = 0; p < 4; ++p) {
				if (!tiles[i][j].HasDirection(GameDefs::DIRECTIONS[p])) {
					float wallX = x;
					float wallZ = z;
					float scaleX = 1;
					float scaleZ = 1;
					
					auto delta = GameDefs::DIRECTION_CHANGES[p];
					wallX += delta.second;
					wallZ += delta.first;

					scaleX -= abs(delta.second)  * 0.9;
					scaleZ -= abs(delta.first) * 0.9;

					XMMATRIX modelMat = XMMatrixTranslation(wallX, y, wallZ);
					modelMat = XMMatrixScaling(scaleX, 1, scaleZ) * modelMat;
					mvpMatrices.push_back(modelMat);
				}
			}

			x += 2;
		}
		z += 2;
	}
	m_numInstances = mvpMatrices.size();
	// Create a buffer and upload the MVP matrices to the GPU
	D3D12_SUBRESOURCE_DATA mvpData = {};
	mvpData.pData = mvpMatrices.data();
	mvpData.RowPitch = sizeof(XMMATRIX) * m_numInstances;
	mvpData.SlicePitch = mvpData.RowPitch;

	auto commandList = m_commQueue->GetCommandList();

	ComPtr<ID3D12Resource> intermediateBuffer;
	UpdateBufferResource(commandList.Get(),
		&m_modelBuffer, &intermediateBuffer,
		m_numInstances, sizeof(XMMATRIX), mvpMatrices.data());

	m_modelBufferView.BufferLocation = m_modelBuffer->GetGPUVirtualAddress();
	m_modelBufferView.SizeInBytes = (UINT)(sizeof(XMMATRIX) * m_numInstances);
	m_modelBufferView.StrideInBytes = sizeof(XMMATRIX);
	auto fenceValue = m_commQueue->ExecuteCommandList(commandList);
	m_commQueue->WaitForFenceValue(fenceValue);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = m_numInstances;
	srvDesc.Buffer.StructureByteStride = sizeof(XMMATRIX);
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	m_device->CreateShaderResourceView(m_modelBuffer.Get(), &srvDesc, m_cbvSrvUavHeap->GetCPUDescriptorHandleForHeapStart());
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
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	// For MVP
	CD3DX12_ROOT_PARAMETER1 rootParameters[2];
	rootParameters[0].InitAsConstants(sizeof(DirectX::XMMATRIX) / 4, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsShaderResourceView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDescription;
	rootSignatureDescription.Init_1_1(_countof(rootParameters), rootParameters, 0, nullptr, rootSignatureFlags);

	// Serialize the root signature.
	ComPtr<ID3DBlob> rootSignatureBlob;
	ComPtr<ID3DBlob> errorBlob;
	ThrowIfFailed(D3DX12SerializeVersionedRootSignature(&rootSignatureDescription,
		featureData.HighestVersion, &rootSignatureBlob, &errorBlob));
	// Create the root signature.
	ThrowIfFailed(m_device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(),
		rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

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

	pipelineStateStream.pRootSignature = m_rootSignature.Get();
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

	auto commandList = m_commQueue->GetCommandList();
	auto fenceValue = m_commQueue->ExecuteCommandList(commandList);
	m_commQueue->WaitForFenceValue(fenceValue);
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
		m_dsvHeap->GetCPUDescriptorHandleForHeapStart());
}

void Renderer_D12::UpdateMVP(float fov, DirectX::XMVECTOR camPos, DirectX::XMVECTOR camFwd, DirectX::XMVECTOR camRight, DirectX::XMVECTOR camUp) {
	// Update the view matrix.
	const XMVECTOR upDirection = XMVector3Cross(camFwd, camRight);
	m_viewMatrix = XMMatrixLookAtLH(camPos, camPos + camFwd, upDirection);

	// Update the projection matrix.
	float aspectRatio = GAME_WINDOW->GetWidth() / static_cast<float>(GAME_WINDOW->GetHeight());
	m_projectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(fov), aspectRatio, 0.1f, 100.0f);
}

ComPtr<ID3D12Device2> Renderer_D12::GetDevice() const {
	return m_device;
}

uint64_t  Renderer_D12::GetCurrentFrameCount() const {
	return m_currentFrame;
}

// Helper functions
	// Transition a resource
void Renderer_D12::TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	commandList->ResourceBarrier(1, &barrier);
}

// Clear a render target view.
void Renderer_D12::ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor) {
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

// Clear the depth of a depth-stencil view.
void Renderer_D12::ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth) {
	commandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

// Create a GPU buffer.
void Renderer_D12::UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
	ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags) {

	//Create the buffer
	size_t bufferSize = numElements * elementSize;
	ThrowIfFailed(m_device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	if (bufferData)
	{
		//Create an intermediate buffer to upload the data
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));        
		
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		//Copy the data
		UpdateSubresources(commandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer_D12::GetCurrentRenderTargetView() const
{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
		m_currentBufferIdx, m_rtvDescSize);
}