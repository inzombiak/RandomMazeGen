#include "DescriptorAllocator_D12.h"
#include "DescriptorAllocatorPage_D12.h"

#include <algorithm>

DescriptorAllocator_D12::DescriptorAllocator_D12(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptorsPerHeap)
    : m_heapType(type)
    , m_numDescriptorsPerHeap(numDescriptorsPerHeap)
{
}

DescriptorAllocator_D12::~DescriptorAllocator_D12() 
{

}

DescriptorAllocation_D12 DescriptorAllocator_D12::Allocate(uint32_t numDescriptors)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);

    DescriptorAllocation_D12 allocation;

    for (auto iter = m_availableHeaps.begin(); iter != m_availableHeaps.end(); ++iter)
    {
        auto allocatorPage = m_heapPool[*iter];

        allocation = allocatorPage->Allocate(numDescriptors);

        if (allocatorPage->NumFreeHandles() == 0)
        {
            iter = m_availableHeaps.erase(iter);
        }

        // A valid allocation has been found.
        if (!allocation.IsNull())
        {
            break;
        }
    }

    // No available heap could satisfy the requested number of descriptors.
    if (allocation.IsNull())
    {
        // Create a page large enough for this allocation, but don't permanently
        // increase the default page size. This prevents memory waste from one
        // large allocation causing all future pages to be oversized.
        auto pageSize = std::max(m_numDescriptorsPerHeap, numDescriptors);
        auto newPage = CreateAllocatorPage(pageSize);

        allocation = newPage->Allocate(numDescriptors);
    }

    return allocation;
}

void DescriptorAllocator_D12::ReleaseStaleDescriptors(uint64_t frameNumber)
{
    std::lock_guard<std::mutex> lock(m_allocationMutex);

    for (size_t i = 0; i < m_heapPool.size(); ++i)
    {
        auto page = m_heapPool[i];

        page->ReleaseStaleDescriptors(frameNumber);

        if (page->NumFreeHandles() > 0)
        {
            m_availableHeaps.insert(i);
        }
    }
}

std::shared_ptr<DescriptorAllocatorPage_D12> DescriptorAllocator_D12::CreateAllocatorPage(uint32_t numDescriptors)
{
    // If numDescriptors is 0, use the default page size
    if (numDescriptors == 0) {
        numDescriptors = m_numDescriptorsPerHeap;
    }

    m_heapPool.emplace_back(std::make_shared<DescriptorAllocatorPage_D12>(m_heapType, numDescriptors));
    m_availableHeaps.insert(m_heapPool.size() - 1);

    return m_heapPool.back();
}
