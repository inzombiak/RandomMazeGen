#pragma once

#include "Rendering/DX12_Helpers.h"
#include "Rendering/Texture_D12.h"
#include "Rendering/RootSignature_D12.h"

#include "d3d12.h"

struct PipelineStateObject {
	std::shared_ptr<RootSignature_D12>  rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			pipelineState;
};

struct Material
{
	std::string name;

	std::wstring vertexShader;
	std::wstring pixelShader;

	PipelineStateObject* pso;
	std::vector<std::shared_ptr<Texture_D12>> textures;
};

