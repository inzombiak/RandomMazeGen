#include "DebugRenderer_D12.h"

#include "CommandList_D12.h"
#include "DX12_Helpers.h"
#include "RootSignature_D12.h"
#include "d3dx12/d3dx12.h"
#include "../Material.h"
#include "RenderDefs.h"

#include <cstring>

const size_t DebugRenderer_D12::MAX_LINES = 200000;

bool DebugRenderer_D12::Initialize(Microsoft::WRL::ComPtr<ID3D12Device2> device, PipelineStateObject* pso)
{
	if (!device || !pso)
		return false;

	m_pso = pso;
	m_capacityVertices = MAX_LINES * 2;

	const size_t byteSize = m_capacityVertices * sizeof(LineVertex);

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_vertexBuffer)));

	NAME_D3D12_OBJECT(m_vertexBuffer);

	CD3DX12_RANGE readRange(0, 0);   // never read back on the CPU
	ThrowIfFailed(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_mapped)));

	m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
	m_vertexBufferView.StrideInBytes  = sizeof(LineVertex);
	m_vertexBufferView.SizeInBytes    = (UINT)byteSize;

	m_vertices.reserve(4096);
	return true;
}

void DebugRenderer_D12::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color)
{
	if (m_vertices.size() + 2 > m_capacityVertices)
		return;   // silently drop past capacity rather than growing mid-frame

	m_vertices.push_back({ from, color });
	m_vertices.push_back({ to,   color });
}

void DebugRenderer_D12::DrawPoint(const glm::vec3& p, float size, const glm::vec3& color)
{
	// Three axis-aligned crosshairs; there is no point topology in this PSO and a
	// cross reads better at depth than a single pixel would.
	const float h = size * 0.5f;
	DrawLine(p - glm::vec3(h, 0.0f, 0.0f), p + glm::vec3(h, 0.0f, 0.0f), color);
	DrawLine(p - glm::vec3(0.0f, h, 0.0f), p + glm::vec3(0.0f, h, 0.0f), color);
	DrawLine(p - glm::vec3(0.0f, 0.0f, h), p + glm::vec3(0.0f, 0.0f, h), color);
}

void DebugRenderer_D12::DrawAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec3& color)
{
	const glm::vec3 c[8] = {
		{ min.x, min.y, min.z }, { max.x, min.y, min.z },
		{ max.x, max.y, min.z }, { min.x, max.y, min.z },
		{ min.x, min.y, max.z }, { max.x, min.y, max.z },
		{ max.x, max.y, max.z }, { min.x, max.y, max.z },
	};

	static const int edges[12][2] = {
		{0,1},{1,2},{2,3},{3,0},   // -Z face
		{4,5},{5,6},{6,7},{7,4},   // +Z face
		{0,4},{1,5},{2,6},{3,7},   // connecting
	};

	for (const auto& e : edges)
		DrawLine(c[e[0]], c[e[1]], color);
}

void DebugRenderer_D12::Record(std::shared_ptr<CommandList_D12> commandList,
                               D3D12_GPU_VIRTUAL_ADDRESS sceneDataAddress)
{
	if (!m_pso || !m_mapped || m_vertices.empty() || !commandList)
		return;

	const size_t count = m_vertices.size();
	std::memcpy(m_mapped, m_vertices.data(), count * sizeof(LineVertex));

	auto cmd = commandList->GetGraphicsCommandList();
	cmd->SetPipelineState(m_pso->pipelineState.Get());
	cmd->SetGraphicsRootSignature(m_pso->rootSignature->GetD3D12RootSignature().Get());

	BindingInfo bi;
	if (m_pso->resourceToBindingInfo.contains(SCENE_DATA_BUFFER_NAME))
	{
		bi = m_pso->resourceToBindingInfo[SCENE_DATA_BUFFER_NAME];
		cmd->SetGraphicsRootConstantBufferView(bi.rootIndex, sceneDataAddress);
	}

	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	cmd->IASetVertexBuffers(0, 1, &m_vertexBufferView);
	cmd->DrawInstanced((UINT)count, 1, 0, 0);

	// Restore triangle topology for anything recorded after us.
	cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
