#include "CommandQueue_D12.h"

#include <assert.h>

CommandQueue_D12::CommandQueue_D12(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type, DWORD fenceTimeout) : m_d3d12Device(device), m_commandListType(type), 
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

CommandQueue_D12::~CommandQueue_D12() {
	::CloseHandle(m_fenceEvent);
}

ComPtr<ID3D12CommandAllocator> CommandQueue_D12::CreateCommandAllocator() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(m_d3d12Device->CreateCommandAllocator(m_commandListType, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}

std::shared_ptr<CommandList_D12> CommandQueue_D12::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator) {
	std::shared_ptr<CommandList_D12> commandList = std::make_shared<CommandList_D12>(m_commandListType);
	return commandList;
}

std::shared_ptr<CommandList_D12> CommandQueue_D12::GetCommandList() {
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	std::shared_ptr<CommandList_D12> commandList;

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
		commandList->Reset(commandAllocator);
	}
	else
	{
		commandList = CreateCommandList(commandAllocator);
	}

	// Associate the command allocator with the command list so that it can be
	// retrieved when the command list is executed.

	return commandList;
}

uint64_t CommandQueue_D12::ExecuteCommandList(std::shared_ptr<CommandList_D12> commandList) {
	commandList->Close();

	auto commandAllocator = commandList->GetCommandAllocator();
	UINT dataSize = sizeof(commandAllocator);

	ID3D12CommandList* const ppCommandLists[] = {
		commandList->GetGraphicsCommandList().Get()
	};

	m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	m_commandAllocatorQueue.emplace(CommandAllocatorEntry{ fenceValue, commandAllocator.Get()});
	m_commandListQueue.push(commandList);

	// The ownership of the command allocator has been transferred to the ComPtr
	// in the command allocator queue. It is safe to release the reference 
	// in this temporary COM pointer here.
	commandAllocator->Release();

	return fenceValue;
}

uint64_t CommandQueue_D12::Signal() {
	uint64_t fenceValueForSignal = ++m_fenceValue;
	ThrowIfFailed(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}
bool CommandQueue_D12::IsFenceComplete(uint64_t fenceValue) {
	return m_d3d12Fence->GetCompletedValue() >= fenceValue;
}
void CommandQueue_D12::WaitForFenceValue(uint64_t fenceValue) {
	if (!IsFenceComplete(fenceValue))
	{
		ThrowIfFailed(m_d3d12Fence->SetEventOnCompletion(fenceValue, m_fenceEvent));
		::WaitForSingleObject(m_fenceEvent, m_fenceTimeout);
	}
}
void CommandQueue_D12::Flush() {
	uint64_t fenceValueForSignal = Signal();
	WaitForFenceValue(fenceValueForSignal);
}

ComPtr<ID3D12CommandQueue> CommandQueue_D12::GetD3D12CommandQueue() const {
	return m_d3d12CommandQueue;
}