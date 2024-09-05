#ifndef RENDERER_D12_H
#define RENDERER_D12_H

// DirectX 12 specific headers.
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include "d3dx12/d3dx12.h"


// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

#include "CommandQueue.h"
struct VertexInput
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
};

class Tile;
class Renderer_D12 {

	public:
		Renderer_D12();
		void ResizeTargets();
		void UpdateRenderTargetViews();
		void Render();
		void Shutdown();
		bool IsInitialized() const;

		void PopulateVertexBuffer(const VertexInput *data, size_t count);
		void PopulateIndexBuffer(const WORD *data, size_t count);
		void BuildPipelineState(const std::wstring& vertexShaderName, const std::wstring& pixelShaderName);

		void CreateSRVForBoxes(const std::vector<std::vector<Tile>>& tiles, double t);

		// Resize the depth buffer to match the size of the client area.
		void ResizeDepthBuffer(int width, int height);

		void UpdateMVP(float fov, DirectX::XMVECTOR camPos, DirectX::XMVECTOR camFwd, DirectX::XMVECTOR camRight, DirectX::XMVECTOR camUp);

	private:

		// Helper functions
		// Transition a resource
		void TransitionResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			Microsoft::WRL::ComPtr<ID3D12Resource> resource,
			D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

		// Clear a render target view.
		void ClearRTV(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

		// Clear the depth of a depth-stencil view.
		void ClearDepth(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

		// Create a GPU buffer.
		void UpdateBufferResource(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList2> commandList,
			ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
			size_t numElements, size_t elementSize, const void* bufferData,
			D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

		Renderer_D12(const Renderer_D12&) = delete;
		Renderer_D12& operator=(const Renderer_D12&) = delete;

		static const uint8_t NUM_BACKBUFFER_FRAMES = 3;

		UINT		m_rtvDescSize;
		UINT		m_currentBufferIdx;

		ComPtr<ID3D12Device2>				m_device;
		std::shared_ptr<CommandQueue>		m_commQueue;
		ComPtr<IDXGISwapChain4>				m_swapChain;
		ComPtr<ID3D12Resource>				m_backbuffers[NUM_BACKBUFFER_FRAMES];
		ComPtr<ID3D12DescriptorHeap>		m_rtvHeap;

		ComPtr<ID3D12DescriptorHeap>		m_cbvSrvUavHeap;

		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		// Index buffer for the cube.
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

		int m_numInstances = 0;
		ComPtr<ID3D12Resource> m_modelBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_modelBufferView;
		
		ComPtr<ID3D12Resource> m_colorBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_colorBufferView;

		ComPtr<ID3D12Resource> m_depthBuffer;
		// Descriptor heap for depth buffer.
		ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
		// Root signature
		ComPtr<ID3D12RootSignature> m_rootSignature;
		// Pipeline state object.
		ComPtr<ID3D12PipelineState> m_pipelineState;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;

		size_t m_indexCount;
		DirectX::XMMATRIX m_viewMatrix;
		DirectX::XMMATRIX m_projectionMatrix;

		//Fencing
		uint64_t			m_fenceValue = 0;
		uint64_t			m_perFrameFenceValues[NUM_BACKBUFFER_FRAMES] = {};

		bool m_tearingSupported = false;
		bool m_initalized = false;
};

#endif