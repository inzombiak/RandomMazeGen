#ifndef TEXTURE_D12_H
#define TEXTURE_D12_H

#include "d3dx12/d3dx12.h"
#include "DescriptorAllocation_D12.h"

class Texture_D12 {

public:
	Texture_D12();

	void SetResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
	Microsoft::WRL::ComPtr<ID3D12Resource> GetResource() const;

	void SetCPUAllocation(const SRVAllocation& cpu);
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const;

	void SetReady(bool ready);
	bool IsReady() const;

private:
	bool m_ready;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	SRVAllocation m_cpuAllocation;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
};

#endif