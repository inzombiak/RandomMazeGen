#include "Texture_D12.h"

Texture_D12::Texture_D12() {
	m_ready = false;
	m_resource = nullptr;
}
void Texture_D12::setResource(Microsoft::WRL::ComPtr<ID3D12Resource> resource) {
	m_resource = resource;
}
Microsoft::WRL::ComPtr<ID3D12Resource> Texture_D12::getResource() const {
	return m_resource;
}

void Texture_D12::setHandles(CD3DX12_CPU_DESCRIPTOR_HANDLE& cpu, CD3DX12_GPU_DESCRIPTOR_HANDLE& gpu) {
	m_cpuHandle = cpu;
	m_gpuHandle = gpu;
}
CD3DX12_CPU_DESCRIPTOR_HANDLE Texture_D12::getCPUHandle() const {
	return m_cpuHandle;
}
CD3DX12_GPU_DESCRIPTOR_HANDLE Texture_D12::getGPUHandle() const {
	return m_gpuHandle;
}

void Texture_D12::setReady(bool state) {
	m_ready = state;
}
bool Texture_D12::isReady() const {
	return m_ready;
}
