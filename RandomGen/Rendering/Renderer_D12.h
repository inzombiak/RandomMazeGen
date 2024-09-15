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

#include "CommandQueue_D12.h"
struct VertexInput
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 color;
	DirectX::XMFLOAT3 uv;
};

class Tile;
class UploadBuffer_D12;
class DescriptorAllocator_D12;
class DescriptorAllocation_D12;
class DynamicDecriptorHeap_D12;
class RootSignature_D12;
class Texture_D12;
class Renderer_D12 {

	public:
		Renderer_D12();
		void PostInit();
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

		ComPtr<ID3D12Device2> GetDevice() const;
		uint64_t GetCurrentFrameCount() const;
		uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		D3D_ROOT_SIGNATURE_VERSION GetHighestRootSigVer() const;

	private:
		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

		Renderer_D12(const Renderer_D12&) = delete;
		Renderer_D12& operator=(const Renderer_D12&) = delete;

		static const uint8_t NUM_BACKBUFFER_FRAMES = 3;

		UINT		m_currentBufferIdx;

		ComPtr<ID3D12Device2>				m_device;
		std::shared_ptr<CommandQueue_D12>	m_commQueue;
		ComPtr<IDXGISwapChain4>				m_swapChain;
		ComPtr<ID3D12Resource>				m_backbuffers[NUM_BACKBUFFER_FRAMES];

		D3D_ROOT_SIGNATURE_VERSION m_highestRootSignatureVersion;
		
		std::shared_ptr<RootSignature_D12>		  m_rootSignature;

		std::shared_ptr<DescriptorAllocator_D12>  m_rtvAllocator;
		std::shared_ptr<DescriptorAllocation_D12> m_rtvs;
		std::shared_ptr<DescriptorAllocator_D12>  m_dsvAllocator;
		std::shared_ptr<DescriptorAllocation_D12> m_dsvs;

		ComPtr<ID3D12DescriptorHeap>			 m_cbvSrvUavHeap;
		std::shared_ptr<DynamicDecriptorHeap_D12> m_cbvSrvUavDynHeap;

		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		// Index buffer for the cube.
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

		int m_numInstances = 0;
		ComPtr<ID3D12Resource> m_modelBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_modelBufferView;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_modelCPUHandle;
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_modelGPUHandle;

		ComPtr<ID3D12Resource> m_colorBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_colorBufferView;

		ComPtr<ID3D12Resource> m_depthBuffer;
		// Pipeline state object.
		ComPtr<ID3D12PipelineState> m_pipelineState;

		std::shared_ptr<Texture_D12> m_wallTexture;
		std::shared_ptr<Texture_D12> m_grassTexture;
		std::shared_ptr<Texture_D12> m_dirtTexture;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;

		size_t m_indexCount;
		DirectX::XMMATRIX m_viewMatrix;
		DirectX::XMMATRIX m_projectionMatrix;

		//Fencing
		uint64_t			m_fenceValue = 0;
		uint64_t			m_perFrameFenceValues[NUM_BACKBUFFER_FRAMES] = {};

		uint64_t			m_currentFrame = 0;

		bool m_tearingSupported = false;
		bool m_initalized = false;
};

#endif