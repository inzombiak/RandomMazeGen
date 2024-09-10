#include "DescriptorAllocatorPage_D12.h"
#include "Renderer_D12.h"

#include "../GameDefs.h"

DescriptorAllocatorPage_D12::DescriptorAllocatorPage_D12(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    : m_heapType(type)
    , m_numDescriptorsInHeap(numDescriptors)
{
    auto device = RENDERER->GetDevice();

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.Type = m_heapType;
    heapDesc.NumDescriptors = m_numDescriptorsInHeap;

    ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap)));

    m_baseDescriptor = m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    m_descriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_heapType);
    m_numFreeHandles = m_numDescriptorsInHeap;

    // Initialize the free lists
    AddNewBlock(0, m_numFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage_D12::GetHeapType() const
{
    return m_heapType;
}

uint32_t DescriptorAllocatorPage_D12::NumFreeHandles() const
{
    return m_numFreeHandles;
}

bool DescriptorAllocatorPage_D12::HasSpace(uint32_t numDescriptors) const
{
    return m_freeListBySize.lower_bound(numDescriptors) != m_freeListBySize.end();
}

void DescriptorAllocatorPage_D12::AddNewBlock(uint32_t offset, uint32_t numDescriptors)
{
    auto offsetIt = m_freeListByOffset.emplace(offset, numDescriptors);
    auto sizeIt = m_freeListBySize.emplace(numDescriptors, offsetIt.first);
    offsetIt.first->second.freeListBySizeIter = sizeIt;
}

DescriptorAllocation_D12 DescriptorAllocatorPage_D12::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);
    // There are less than the requested number of descriptors left in the heap.
    // Return a NULL descriptor and try another heap.
    if (numDescriptors > m_numFreeHandles)
    {
        return DescriptorAllocation_D12();
    }

    // Get the first block that is large enough to satisfy the request.
    auto smallestBlockIt = m_freeListBySize.lower_bound(numDescriptors);
    if (smallestBlockIt == m_freeListBySize.end())
    {
        // There was no free block that could satisfy the request.
        return DescriptorAllocation_D12();
    }
    // The size of the smallest block that satisfies the request.
    auto blockSize = smallestBlockIt->first;

    // The pointer to the same entry in the FreeListByOffset map.
    auto offsetIt = smallestBlockIt->second;

    // The offset in the descriptor heap.
    auto offset = offsetIt->first;
    // Remove the existing free block from the free list.
    m_freeListBySize.erase(smallestBlockIt);
    m_freeListByOffset.erase(offsetIt);

    // Compute the new free block that results from splitting this block.
    auto newOffset = offset + numDescriptors;
    auto newSize = blockSize - numDescriptors;

    if (newSize > 0)
    {
        // If the allocation didn't exactly match the requested size,
        // return the left-over to the free list.
        AddNewBlock(newOffset, newSize);
    }
    // Decrement free handles.
    m_numFreeHandles -= numDescriptors;

    return DescriptorAllocation_D12(
        CD3DX12_CPU_DESCRIPTOR_HANDLE(m_baseDescriptor, offset, m_descriptorHandleIncrementSize),
        numDescriptors, m_descriptorHandleIncrementSize, shared_from_this());
}

void DescriptorAllocatorPage_D12::Free(DescriptorAllocation_D12&& descriptor, uint64_t frameNumber)
{
    // Compute the offset of the descriptor within the descriptor heap.
    auto offset = ComputeOffset(descriptor.GetDescriptorHandle());

    std::lock_guard<std::mutex> lock(m_allocationMutex);

    // Don't add the block directly to the free list until the frame has completed.
    m_staleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

uint32_t DescriptorAllocatorPage_D12::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    return static_cast<uint32_t>(handle.ptr - m_baseDescriptor.ptr) / m_descriptorHandleIncrementSize;
}

//@ZGTODO Read this again
void DescriptorAllocatorPage_D12::FreeBlock(uint32_t offset, uint32_t numDescriptors)
{
    // Find the first element whose offset is greater than the specified offset.
    // This is the block that should appear after the block that is being freed.
    auto nextBlockIt = m_freeListByOffset.upper_bound(offset);

    // Find the block that appears before the block being freed.
    auto prevBlockIt = nextBlockIt;
    // If it's not the first block in the list.
    if (prevBlockIt != m_freeListByOffset.begin())
    {
        // Go to the previous block in the list.
        --prevBlockIt;
    }
    else
    {
        // Otherwise, just set it to the end of the list to indicate that no
        // block comes before the one being freed.
        prevBlockIt = m_freeListByOffset.end();
    }

    // Add the number of free handles back to the heap.
    // This needs to be done before merging any blocks since merging
    // blocks modifies the numDescriptors variable.
    m_numFreeHandles += numDescriptors;

    if (prevBlockIt != m_freeListByOffset.end() &&
        offset == prevBlockIt->first + prevBlockIt->second.size)
    {
        // The previous block is exactly behind the block that is to be freed.
        //
        // PrevBlock.Offset           Offset
        // |                          |
        // |<-----PrevBlock.Size----->|<------Size-------->|
        //

        // Increase the block size by the size of merging with the previous block.
        offset = prevBlockIt->first;
        numDescriptors += prevBlockIt->second.size;

        // Remove the previous block from the free list.
        m_freeListBySize.erase(prevBlockIt->second.freeListBySizeIter);
        m_freeListByOffset.erase(prevBlockIt);
    }

    if (nextBlockIt != m_freeListByOffset.end() &&
        offset + numDescriptors == nextBlockIt->first)
    {
        // The next block is exactly in front of the block that is to be freed.
        //
        // Offset               NextBlock.Offset 
        // |                    |
        // |<------Size-------->|<-----NextBlock.Size----->|

        // Increase the block size by the size of merging with the next block.
        numDescriptors += nextBlockIt->second.size;

        // Remove the next block from the free list.
        m_freeListBySize.erase(nextBlockIt->second.freeListBySizeIter);
        m_freeListByOffset.erase(nextBlockIt);
    }
    // Add the freed block to the free list.
    AddNewBlock(offset, numDescriptors);
}

void DescriptorAllocatorPage_D12::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);

    while (!m_staleDescriptors.empty() && m_staleDescriptors.front().frameNumber <= frameNumber)
    {
        auto& staleDescriptor = m_staleDescriptors.front();

        // The offset of the descriptor in the heap.
        auto offset = staleDescriptor.offset;
        // The number of descriptors that were allocated.
        auto numDescriptors = staleDescriptor.size;

        FreeBlock(offset, numDescriptors);

        m_staleDescriptors.pop();
    }
}