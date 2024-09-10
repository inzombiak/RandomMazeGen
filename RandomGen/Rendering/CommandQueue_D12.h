#ifndef COMMAND_QUEUE_D12_H
#define COMMAND_QUEUE_D12_H

#include <d3d12.h>  // For ID3D12CommandQueue, ID3D12Device2, and ID3D12Fence
#include <wrl.h>    // For Microsoft::WRL::ComPtr

#include <cstdint>  // For uint64_t
#include <queue>    // For std::queue
#include <memory>

using Microsoft::WRL::ComPtr;

#include "DX12_Helpers.h"
#include "CommandList_D12.h"

class CommandQueue_D12
{
public:
    CommandQueue_D12(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type, DWORD fenceTimeout = DWORD_MAX);
    virtual ~CommandQueue_D12();

    // Get an available command list from the command queue.
    std::shared_ptr<CommandList_D12> GetCommandList();

    // Execute a command list.
    // Returns the fence value to wait for for this command list.
    uint64_t ExecuteCommandList(std::shared_ptr<CommandList_D12> commandList);

    uint64_t Signal();
    bool IsFenceComplete(uint64_t fenceValue);
    void WaitForFenceValue(uint64_t fenceValue);
    void Flush();

    ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const;
protected:

    ComPtr<ID3D12CommandAllocator> CreateCommandAllocator();
    std::shared_ptr<CommandList_D12> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator);

private:
    // Keep track of command allocators that are "in-flight"
    struct CommandAllocatorEntry
    {
        uint64_t fenceValue;
        ComPtr<ID3D12CommandAllocator> commandAllocator;
    };

    using CommandAllocatorQueue = std::queue<CommandAllocatorEntry>;
    using CommandListQueue = std::queue<std::shared_ptr<CommandList_D12>>;

    D3D12_COMMAND_LIST_TYPE                     m_commandListType;
    ComPtr<ID3D12Device2>                       m_d3d12Device;
    ComPtr<ID3D12CommandQueue>                  m_d3d12CommandQueue;
    ComPtr<ID3D12Fence>                         m_d3d12Fence;
    HANDLE                                      m_fenceEvent;
    uint64_t                                    m_fenceValue;

    CommandAllocatorQueue                       m_commandAllocatorQueue;
    CommandListQueue                            m_commandListQueue;

    DWORD                                       m_fenceTimeout;
};

#endif