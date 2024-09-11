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

std::shared_ptr<CommandList_D12> CommandQueue_D12::CreateCommandList() {
	std::shared_ptr<CommandList_D12> commandList = std::make_shared<CommandList_D12>(m_d3d12Device, m_commandListType);
	
	return commandList;
}

std::shared_ptr<CommandList_D12> CommandQueue_D12::GetCommandList() {
	if (m_activeCommandList)
		return m_activeCommandList;

	std::shared_ptr<CommandList_D12> commandList;

	if (!m_commandListQueue.empty())
	{
		commandList = m_commandListQueue.front();
		m_commandListQueue.pop();
		commandList->Reset();
	}
	else
	{
		commandList = CreateCommandList();
	}

	m_activeCommandList = commandList;
	return commandList;
}

uint64_t CommandQueue_D12::ExecuteActiveCommandList() {
	return ExecuteCommandList(m_activeCommandList);
}

uint64_t CommandQueue_D12::ExecuteCommandList(std::shared_ptr<CommandList_D12> commandList) {
	commandList->Close();


	ID3D12CommandList* const ppCommandLists[] = {
		commandList->GetGraphicsCommandList().Get()
	};

	m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();
	m_commandListQueue.push(commandList);

	if (commandList == m_activeCommandList)
		m_activeCommandList = nullptr;

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