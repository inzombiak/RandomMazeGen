#include "DescriptorAllocation_D12.h"

#include "Renderer_D12.h"
#include "DescriptorAllocatorPage_D12.h"

#include "../GameDefs.h"

DescriptorAllocation_D12::DescriptorAllocation_D12()
    : m_descriptor{ 0 }
    , m_numHandles(0)
    , m_descriptorSize(0)
    , m_page(nullptr)
{}

DescriptorAllocation_D12::DescriptorAllocation_D12(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numHandles, uint32_t descriptorSize, std::shared_ptr<DescriptorAllocatorPage_D12> page)
    : m_descriptor(descriptor)
    , m_numHandles(numHandles)
    , m_descriptorSize(descriptorSize)
    , m_page(page)
{}

DescriptorAllocation_D12::~DescriptorAllocation_D12()
{
    Free();
}

DescriptorAllocation_D12::DescriptorAllocation_D12(DescriptorAllocation_D12&& allocation)
    : m_descriptor(allocation.m_descriptor)
    , m_numHandles(allocation.m_numHandles)
    , m_descriptorSize(allocation.m_descriptorSize)
    , m_page(std::move(allocation.m_page))
{
    allocation.m_descriptor.ptr = 0;
    allocation.m_numHandles = 0;
    allocation.m_descriptorSize = 0;
}

DescriptorAllocation_D12& DescriptorAllocation_D12::operator=(DescriptorAllocation_D12&& other)
{
    // Free this descriptor if it points to anything.
    Free();

    m_descriptor = other.m_descriptor;
    m_numHandles = other.m_numHandles;
    m_descriptorSize = other.m_descriptorSize;
    m_page = std::move(other.m_page);

    other.m_descriptor.ptr = 0;
    other.m_numHandles = 0;
    other.m_descriptorSize = 0;

    return *this;
}

void DescriptorAllocation_D12::Free()
{
    if (!IsNull() && m_page)
    {
        m_page->Free(std::move(*this), RENDERER->GetCurrentFrameCount());

        m_descriptor.ptr = 0;
        m_numHandles = 0;
        m_descriptorSize = 0;
        m_page.reset();
    }
}

// Check if this a valid descriptor.
bool DescriptorAllocation_D12::IsNull() const
{
    return m_descriptor.ptr == 0;
}

// Get a descriptor at a particular offset in the allocation.
D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation_D12::GetDescriptorHandle(uint32_t offset) const
{
    assert(offset < m_numHandles);
    return { m_descriptor.ptr + (m_descriptorSize * offset) };
}

uint32_t DescriptorAllocation_D12::GetNumHandles() const
{
    return m_numHandles;
}

std::shared_ptr<DescriptorAllocatorPage_D12> DescriptorAllocation_D12::GetDescriptorAllocatorPage() const
{
    return m_page;
}