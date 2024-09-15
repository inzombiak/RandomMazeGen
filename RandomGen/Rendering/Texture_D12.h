#ifndef TEXTURE_D12_H
#define TEXTURE_D12_H

#include "d3dx12/d3dx12.h"

class Texture_D12 {

public:
	Texture_D12();

	void setResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource);
	Microsoft::WRL::ComPtr<ID3D12Resource> getResource() const;

	void setHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpu, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpu);
	CD3DX12_CPU_DESCRIPTOR_HANDLE getCPUHandle() const;
	CD3DX12_GPU_DESCRIPTOR_HANDLE getGPUHandle() const;

	void setReady(bool ready);
	bool isReady() const;
private:
	bool m_ready;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_cpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_gpuHandle;
};

#endif