#include "CommandQueue.h"

#include <assert.h>

CommandQueue::CommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type, DWORD fenceTimeout) : m_d3d12Device(device), m_commandListType(type), 
			m_fenceValue(0), m_fenceTimeout(fenceTimeout) {

	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

	m_fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(m_fenceEvent && "Failed to create fence event.");
}

CommandQueue::~CommandQueue() {
	::CloseHandle(m_fenceEvent);
}

ComPtr<ID3D12CommandAllocator> CommandQueue::CreateCommandAllocator() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_d3d12Device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator) {
	ComPtr<ID3D12GraphicsCommandList2> commandList;
	ThrowIfFailed(m_d3d12Device->CreateCommandList(0, m_commandListType, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	return commandList;
}

ComPtr<ID3D12GraphicsCommandList2> CommandQueue::GetCommandList() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12GraphicsCommandList2> commandList;

	if (!m_commandAllocatorQueue.empty() && IsFenceComplete(m_commandAllocatorQueue.front().fenceValue))
	{
		commandAllocator = m_commandAllocatorQueue.front().commandAllocator;
		m_commandAllocatorQueue.pop();

		ThrowIfFailed(commandAllocator->Reset());
	}
	else
	{
		commandAllocator = CreateCommandAllocator();
	}

	if (!m_commandListQueue.empty())
	{
		commandList = m_commandListQueue.front();
		m_commandListQueue.pop();

		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), nullptr));
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.
	ThrowIfFailed(commandList->SetPrivateDataInterface(__uuidof(ID3D12CommandAllocator), commandAllocator.Get()));

	return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(ComPtr<ID3D12GraphicsCommandList2> commandList) {
	commandList->Close();

	ID3D12CommandAllocator* commandAllocator;
	UINT dataSize = sizeof(commandAllocator);
	ThrowIfFailed(commandList->GetPrivateData(__uuidof(ID3D12CommandAllocator), &dataSize, &commandAllocator));

	ID3D12CommandList* const ppCommandLists[] = {
		commandList.Get()
	};

	m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	m_commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator });
	m_commandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64_t CommandQueue::Signal() {
	uint64_t fenceValueForSignal = ++m_fenceValue;
	ThrowIfFailed(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}
bool CommandQueue::IsFenceComplete(uint64_t fenceValue) {
	return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}
void CommandQueue::WaitForFenceValue(uint64_t fenceValue) {
	if (!IsFenceComplete(fenceValue))
	{
		ThrowIfFailed(m_d3d12Fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
		::WaitForSingleObject(m_fenceEvent, m_fenceTimeout);
	}
}
void CommandQueue::Flush() {
	uint64_t fenceValueForSignal = Signal();
	WaitForFenceValue(fenceValueForSignal);
}

ComPtr<ID3D12CommandQueue> CommandQueue::GetD3D12CommandQueue() const {
	return m_d3d12CommandQueue;
}