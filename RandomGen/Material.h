#pragma once

#include "Rendering/DX12_Helpers.h"
#include "Rendering/Texture_D12.h"
#include "Rendering/RootSignature_D12.h"

#include "d3d12.h"

#include <string>
#include <map>

struct BindingInfo {
	uint32_t rootIndex;
	uint32_t offset;
	D3D_SHADER_INPUT_TYPE type;
};

struct PipelineStateObject {
	std::shared_ptr<RootSignature_D12>  rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState>			pipelineState;
	std::map<std::string, BindingInfo> resourceToBindingInfo;
};

struct TextureAttachmentInfo {
	std::string shaderName;
	std::shared_ptr<Texture_D12> texture;
};

struct Material
{
	std::string name;

	std::wstring vertexShader;
	std::wstring pixelShader;

	PipelineStateObject* pso;
	std::vector<TextureAttachmentInfo> textureAttachments;
};

static bool GetMaterialBindingInfoForResource(const Material& mat, const std::string& resourceName, BindingInfo& out) {
	if (!mat.pso->resourceToBindingInfo.contains(resourceName))
		return false;

	out = mat.pso->resourceToBindingInfo[resourceName];
	return true;
}