#include "Texture_D12.h"

Texture_D12::Texture_D12() {
	m_ready = false;
	m_resource = nullptr;
}
void Texture_D12::SetResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
	m_resource = resource;
}
Microsoft::WRL::ComPtr<ID3D12Resource> Texture_D12::GetResource() const {
	return m_resource;
}

void Texture_D12::SetHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpu, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpu) {
	m_cpuHandle = cpu;
	m_gpuHandle = gpu;
}
CD3DX12_CPU_DESCRIPTOR_HANDLE Texture_D12::GetCPUHandle() const {
	return m_cpuHandle;
}
CD3DX12_GPU_DESCRIPTOR_HANDLE Texture_D12::GetGPUHandle() const {
	return m_gpuHandle;
}

void Texture_D12::SetReady(bool state) {
	m_ready = state;
}
bool Texture_D12::IsReady() const {
	return m_ready;
}
