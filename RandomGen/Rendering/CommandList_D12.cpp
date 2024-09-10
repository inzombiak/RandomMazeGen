#include "CommandList_D12.h"

#include "DX12_Helpers.h"

CommandList_D12::CommandList_D12(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type, ComPtr<ID3D12CommandAllocator> allocator)
{
	ThrowIfFailed(device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&m_graphicsCommandList)));
	m_allocator = allocator;
}
// Helper functions
	// Transition a resource
void CommandList_D12::TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	m_graphicsCommandList->ResourceBarrier(1, &barrier);
}

// Clear a render target view.
void CommandList_D12::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor) {
	m_graphicsCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

// Clear the depth of a depth-stencil view.
void CommandList_D12::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth) {
	m_graphicsCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

// Create a GPU buffer.
void CommandList_D12::UpdateBufferResource(ComPtr<ID3D12Device> device, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags) {

	//Create the buffer
	size_t bufferSize = numElements * elementSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	if (bufferData)
	{
		//Create an intermediate buffer to upload the data
		ThrowIfFailed(device->CreateCommittedResource(
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
		UpdateSubresources(m_graphicsCommandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

void CommandList_D12::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap) {

}

void CommandList_D12::Reset(ComPtr<ID3D12CommandAllocator> allocator) {
	m_graphicsCommandList->Reset(allocator.Get(), nullptr);
	//m_allocator = allocator;
}

void CommandList_D12::Close() {
	m_graphicsCommandList->Close();
}

ComPtr<ID3D12CommandAllocator> CommandList_D12::GetCommandAllocator() const 
{
	return m_allocator;
}
ComPtr<ID3D12GraphicsCommandList2> CommandList_D12::GetGraphicsCommandList() const 
{
	return m_graphicsCommandList;
}
