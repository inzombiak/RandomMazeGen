#ifndef COMMAND_LIST_D12_H
#define COMMAND_LIST_D12_H

class CommandList_D12 {

public:
	CommandList_D12(D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12CommandAllocator> allocator);
	//ThrowIfFailed(m_d3d12Device->CreateCommandList(0, m_commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	//ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()))
	
	
	// Helper functions
	// Transition a resource
	void TransitionResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ComPtr<ID3D12Resource> resource,
		D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

	// Clear a render target view.
	void ClearRTV(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor);

	// Clear the depth of a depth-stencil view.
	void ClearDepth(ComPtr<ID3D12GraphicsCommandList2> commandList,
		D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth = 1.0f);

	// Create a GPU buffer.
	void UpdateBufferResource(ComPtr<ID3D12GraphicsCommandList2> commandList,
		ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
		size_t numElements, size_t elementSize, const void* bufferData,
		D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap);

	void Reset(ComPtr<ID3D12CommandAllocator> allocator);
	//commandList->Reset(commandAllocator.Get(), nullptr));
	//ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	void Close();

	ComPtr<ID3D12CommandAllocator> GetCommandAllocator() const;
	ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const;

private:


};
#endif