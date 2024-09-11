#ifndef COMMAND_LIST_D12_H
#define COMMAND_LIST_D12_H

#include "d3dx12/d3dx12.h"
#include "d3d12.h"

#include <wrl.h>
using namespace Microsoft::WRL;

class CommandList_D12 {

public:
	CommandList_D12(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type);
	
	// Helper functions
	// Transition a resource
	void TransitionResource(ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	// Clear a render target view.
	void ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	// Clear the depth of a depth-stencil view.
	void ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	// Create a GPU buffer.
	void UpdateBufferResource(ComPtr<ID3D12Device> device, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

	void Reset();
	//commandList->Reset(commandAllocator.Get(), nullptr));
	//ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	void Close();

	ComPtr<ID3D12CommandAllocator>		GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList2>	GetGraphicsCommandList() const;

	void TrackResource(ComPtr<ID3D12Resource> resource);

private:
	ComPtr<ID3D12GraphicsCommandList2>	m_graphicsCommandList;
	ComPtr<ID3D12CommandAllocator>		m_allocator;

	std::vector<ComPtr<ID3D12Resource>> m_trackedResources;
};
#endif