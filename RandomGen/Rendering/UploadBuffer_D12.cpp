#include "UploadBuffer_D12.h"

#include "Renderer_D12.h"
#include "DX12_Helpers.h"
#include "../GameDefs.h"

#include "d3dx12/d3dx12.h"

#include <new> // for std::bad_alloc

UploadBuffer_D12::UploadBuffer_D12(size_t pageSize)
    : m_pageSize(pageSize)
{}

UploadBuffer_D12::Allocation UploadBuffer_D12::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (sizeInBytes > m_pageSize)
    {
        throw std::bad_alloc();
    }
     
    if (!m_currentPage || !m_currentPage->HasSpace(sizeInBytes, alignment))
    {
        m_currentPage = RequestPage();
    }

    return m_currentPage->Allocate(sizeInBytes, alignment);
}

void UploadBuffer_D12::Reset()
{
    m_currentPage = nullptr;

    // Reset all newly generated pages.
    m_availablePages = m_pagePool;

    for (auto page : m_availablePages)
    {
        // Reset the page for new allocations.
        page->Reset();
    }
}

std::shared_ptr<UploadBuffer_D12::Page> UploadBuffer_D12::RequestPage()
{
    std::shared_ptr<Page> page;

    if (!m_availablePages.empty())
    {
        page = m_availablePages.front();
        m_availablePages.pop_front();
    }
    else
    {
        page = std::make_shared<Page>(m_pageSize);
        m_pagePool.push_back(page);
    }

    return page;
}

UploadBuffer_D12::Page::Page(size_t sizeInBytes)
    : m_pageSize(sizeInBytes)
    , m_offset(0)
    , m_cpuData(nullptr)
    , m_gpuData(D3D12_GPU_VIRTUAL_ADDRESS(0))
{
    auto device = RENDERER->GetDevice();

    ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(m_pageSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_d3d12Resource)
    ));

    m_gpuData = m_d3d12Resource->GetGPUVirtualAddress();
    m_d3d12Resource->Map(0, nullptr, &m_cpuData);
}

UploadBuffer_D12::Page::~Page()
{
    m_d3d12Resource->Unmap(0, nullptr);
    m_cpuData = nullptr;
    m_gpuData = D3D12_GPU_VIRTUAL_ADDRESS(0);
}

bool UploadBuffer_D12::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
    //@ZGTODO Work out this allignment stuff on paper
    size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
    size_t alignedOffset = Math::AlignUp(m_offset, alignment);

    return alignedOffset + alignedSize <= m_pageSize;
}

UploadBuffer_D12::Allocation UploadBuffer_D12::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
    if (!HasSpace(sizeInBytes, alignment))
    {
        // Can't allocate space from page.
        throw std::bad_alloc();
    }

    size_t alignedSize = Math::AlignUp(sizeInBytes, alignment);
    m_offset = Math::AlignUp(m_offset, alignment);

    Allocation allocation;
    allocation.cpu_loc = static_cast<uint8_t*>(m_cpuData) + m_offset;
    allocation.gpu_loc = m_gpuData + m_offset;

    m_offset += alignedSize;

    return allocation;
}

void UploadBuffer_D12::Page::Reset()
{
    m_offset = 0;
}