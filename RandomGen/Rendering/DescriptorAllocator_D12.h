#ifndef DESCRIPTOR_ALLOCATOR_D12_H
#define DESCRIPTOR_ALLOCATOR_D12_H

#include "DescriptorAllocation_D12.h"

#include "d3dx12/d3dx12.h"

#include <cstdint>
#include <mutex>
#include <memory>
#include <set>
#include <vector>

class DescriptorAllocatorPage_D12;
class DescriptorAllocator_D12
{
public:
    DescriptorAllocator_D12(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap = 256);
    virtual ~DescriptorAllocator_D12();

    /**
     * Allocate a number of contiguous descriptors from a CPU visible descriptor heap.
     *
     * @param numDescriptors The number of contiguous descriptors to allocate.
     * Cannot be more than the number of descriptors per descriptor heap.
     */
    DescriptorAllocation_D12 Allocate(uint32_t numDescriptors = 1);

    /**
     * When the frame has completed, the stale descriptors can be released.
     */
    void ReleaseStaleDescriptors(uint64_t frameNumber);

private:
    using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage_D12>>;

    // Create a new heap with a specific number of descriptors.
    std::shared_ptr<DescriptorAllocatorPage_D12> CreateAllocatorPage();

    D3D12_DESCRIPTOR_HEAP_TYPE m_heapType;
    uint32_t m_numDescriptorsPerHeap;

    DescriptorHeapPool m_heapPool;
    // Indices of available heaps in the heap pool.
    std::set<size_t> m_availableHeaps;

    std::mutex m_allocationMutex;
};

#endif