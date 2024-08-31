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

class Renderer_D12 {

	public:
		Renderer_D12();
		void ResizeTargets();
		void UpdateRenderTargetViews();
		void Render();
		void Shutdown();
		bool IsInitialized() const;

	private:

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

		//Fencing
		uint64_t			m_fenceValue = 0;
		uint64_t			m_perFrameFenceValues[NUM_BACKBUFFER_FRAMES] = {};

		bool m_tearingSupported = false;
		bool m_initalized = false;
};

#endif