#ifndef DESCRIPTOR_ALLOCATION_D12_H
#define DESCRIPTOR_ALLOCATION_D12_H
#include <d3d12.h>

#include <cstdint>
#include <memory>

class DescriptorAllocatorPage_D12;
class DescriptorAllocation_D12
{
public:
    // Creates a NULL descriptor.
    DescriptorAllocation_D12();

    DescriptorAllocation_D12(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage_D12> page);

    // The destructor will automatically free the allocation.
    ~DescriptorAllocation_D12();

    // Copies are not allowed.
    DescriptorAllocation_D12(const DescriptorAllocation_D12&) = delete;
    DescriptorAllocation_D12& operator=(const DescriptorAllocation_D12&) = delete;

    // Move is allowed.
    DescriptorAllocation_D12(DescriptorAllocation_D12&& allocation);
    DescriptorAllocation_D12& operator=(DescriptorAllocation_D12&& other);

    // Check if this a valid descriptor.
    bool IsNull() const;

    // Get a descriptor at a particular offset in the allocation.
    D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0) const;

    // Get the number of (consecutive) handles for this allocation.
    uint32_t GetNumHandles() const;
    // Get the heap that this allocation came from.
    // (For internal use only).
    std::shared_ptr<DescriptorAllocatorPage_D12> GetDescriptorAllocatorPage() const;
private:
    // Free the descriptor back to the heap it came from.
    void Free();

    // The base descriptor.
    D3D12_CPU_DESCRIPTOR_HANDLE m_descriptor;
    // The number of descriptors in this allocation.
    uint32_t m_numHandles;
    // The offset to the next descriptor.
    uint32_t m_descriptorSize;

    // A pointer back to the original page where this allocation came from.
    std::shared_ptr<DescriptorAllocatorPage_D12> m_page;
};

#endif