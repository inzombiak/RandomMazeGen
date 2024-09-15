#include "CommandList_D12.h"
#include "UploadBuffer_D12.h"

#include "../Extern/DirectXTex/DirectXTex.h"

#include "DX12_Helpers.h"

CommandList_D12::CommandList_D12(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type)
{
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_allocator)));
	ThrowIfFailed(device->CreateCommandList(0, type, m_allocator.Get(), nullptr, IID_PPV_ARGS(&m_graphicsCommandList)));
	m_uploadBuffer = std::make_shared<UploadBuffer_D12>();
    m_device = device;
}
// Helper functions
	// Transition a resource
void CommandList_D12::TransitionResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource,
	D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState) {

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
		resource.Get(),
		beforeState, afterState);

	m_graphicsCommandList->ResourceBarrier(1, &barrier);
}

// Clear a render target view.
void CommandList_D12::ClearRTV(D3D12_CPU_DESCRIPTOR_HANDLE rtv, FLOAT* clearColor) {
	m_graphicsCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

// Clear the depth of a depth-stencil view.
void CommandList_D12::ClearDepth(D3D12_CPU_DESCRIPTOR_HANDLE dsv, FLOAT depth) {
	m_graphicsCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

// Create a GPU buffer.
void CommandList_D12::UpdateBufferResource(ComPtr<ID3D12Device> device, ID3D12Resource** pDestinationResource, ID3D12Resource** pIntermediateResource,
	size_t numElements, size_t elementSize, const void* bufferData,
	D3D12_RESOURCE_FLAGS flags) {

	//Create the buffer
	size_t bufferSize = numElements * elementSize;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(pDestinationResource)));

	if (bufferData)
	{
		//Create an intermediate buffer to upload the data
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(pIntermediateResource)));

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		//Copy the data
		UpdateSubresources(m_graphicsCommandList.Get(),
			*pDestinationResource, *pIntermediateResource,
			0, 0, 1, &subresourceData);
	}
}

#include <filesystem>
using namespace std;
using namespace DirectX;
void CommandList_D12::LoadTexture(std::wstring filename, ComPtr<ID3D12Resource>& tex, CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle) {

    std::filesystem::path filePath(filename);
    if (!std::filesystem::exists(filePath))
    {
        throw std::exception("File not found.");
    }


    TexMetadata metadata;
    ScratchImage scratchImage;

    if (filePath.extension() == ".dds")
    {
        // Use DDS texture loader.
        ThrowIfFailed(LoadFromDDSFile(filename.c_str(), DDS_FLAGS_NONE, &metadata, scratchImage));
    }
    else if (filePath.extension() == ".hdr")
    {
        ThrowIfFailed(LoadFromHDRFile(filename.c_str(), &metadata, scratchImage));
    }
    else if (filePath.extension() == ".tga")
    {
        ThrowIfFailed(LoadFromTGAFile(filename.c_str(), &metadata, scratchImage));
    }
    else
    {
        ThrowIfFailed(LoadFromWICFile(filename.c_str(), WIC_FLAGS_NONE, &metadata, scratchImage));
    }

    D3D12_RESOURCE_DESC textureDesc = {};
    switch (metadata.dimension)
    {
    case TEX_DIMENSION_TEXTURE1D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT16>(metadata.arraySize));
        break;
    case TEX_DIMENSION_TEXTURE2D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.arraySize));
        break;
    case TEX_DIMENSION_TEXTURE3D:
        textureDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, static_cast<UINT64>(metadata.width), static_cast<UINT>(metadata.height), static_cast<UINT16>(metadata.depth));
        break;
    default:
        throw std::exception("Invalid texture dimension.");
        break;
    }

    ThrowIfFailed(m_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&tex)));

    std::vector<D3D12_SUBRESOURCE_DATA> subresources(scratchImage.GetImageCount());
    const Image* pImages = scratchImage.GetImages();
    for (int i = 0; i < scratchImage.GetImageCount(); ++i)
    {
        auto& subresource = subresources[i];
        subresource.RowPitch = pImages[i].rowPitch;
        subresource.SlicePitch = pImages[i].slicePitch;
        subresource.pData = pImages[i].pixels;
    }

    // Resource must be in the copy-destination state.
    TransitionResource(tex, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

    UINT64 requiredSize = GetRequiredIntermediateSize(tex.Get(), 0, subresources.size());

    // Create a temporary (intermediate) resource for uploading the subresources
    ComPtr<ID3D12Resource> intermediateResource;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(requiredSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediateResource)
    ));

    UpdateSubresources(m_graphicsCommandList.Get(), tex.Get(), intermediateResource.Get(), 0, 0, subresources.size(), subresources.data());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    m_device->CreateShaderResourceView(tex.Get(), &srvDesc, cpuHandle);

    // Resource must be in the copy- state.
    TransitionResource(tex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    TrackResource(intermediateResource);
    TrackResource(tex); 
}
void CommandList_D12::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap) {

}

void CommandList_D12::Reset() {
	m_allocator->Reset();
	m_graphicsCommandList->Reset(m_allocator.Get(), nullptr);
	m_trackedResources.clear();
	m_uploadBuffer->Reset();
}

void CommandList_D12::TrackResource(ComPtr<ID3D12Resource> resource) {
	m_trackedResources.push_back(resource);
}

void CommandList_D12::Close() {
	m_graphicsCommandList->Close();
}

ComPtr<ID3D12CommandAllocator> CommandList_D12::GetCommandAllocator() const 
{
	return m_allocator;
}
ComPtr<ID3D12GraphicsCommandList2> CommandList_D12::GetGraphicsCommandList() const 
{
	return m_graphicsCommandList;
}
