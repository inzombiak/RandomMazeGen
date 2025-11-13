#pragma once
#include "ShaderReflection.h"
#include "d3dx12/d3dx12.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <map>
#include <string>

using Microsoft::WRL::ComPtr;

namespace Rendering {

// Builds a root signature automatically from shader metadata
class RootSignatureBuilder {
public:
    RootSignatureBuilder() = default;

    // Add shader stage metadata (e.g., vertex + pixel)
    void AddShaderStage(const ShaderMetadata& metadata);

    // Add a static sampler (not reflected from shader)
    void AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler);

    // Build the final root signature description (to be passed to RootSignature_D12 constructor)
    bool Build(D3D12_ROOT_SIGNATURE_DESC1& outDesc);

    // Get the mapping of shader resources to root parameter indices
    struct ResourceMapping {
        std::string resourceName;
        UINT rootParameterIndex;
        UINT bindPoint;  // Original shader register
        D3D_SHADER_INPUT_TYPE type;
    };

    const std::vector<ResourceMapping>& GetResourceMappings() const { return m_resourceMappings; }

    // Get root parameter index for a specific resource binding
    int GetRootParameterIndex(const std::string& resourceName) const;
    int GetRootParameterIndexByBindPoint(D3D_SHADER_INPUT_TYPE type, UINT bindPoint) const;

    // Debug: Print the root signature layout
    void PrintLayout() const;

private:
    struct RootParameterInfo {
        CD3DX12_ROOT_PARAMETER1 parameter;
        std::vector<CD3DX12_DESCRIPTOR_RANGE1> ranges; // For descriptor tables
        std::vector<ShaderResourceBinding> sourceBindings; // Track all bindings in this parameter
        D3D12_SHADER_VISIBILITY combinedVisibility;
    };

    std::vector<RootParameterInfo> m_rootParameters;
    std::vector<CD3DX12_ROOT_PARAMETER1> m_builtRootParams;  // Store built params to keep pointers valid
    std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplers;
    std::vector<ResourceMapping> m_resourceMappings;
    std::vector<ShaderResourceBinding> m_allBindings;

    // Merge resources from multiple shader stages
    void MergeResourceBinding(const ShaderResourceBinding& binding);

    // Group consecutive textures/SRVs into descriptor tables
    void OptimizeDescriptorTables();

    // Find existing root parameter that matches a binding
    int FindMatchingRootParameter(const ShaderResourceBinding& binding) const;

    // Create a new root parameter for a binding
    UINT CreateRootParameter(const ShaderResourceBinding& binding);

    // Combine shader visibilities (VS + PS = ALL)
    static D3D12_SHADER_VISIBILITY CombineVisibility(D3D12_SHADER_VISIBILITY a, D3D12_SHADER_VISIBILITY b);

    // Check if two bindings can share a root parameter
    static bool CanMergeBindings(const ShaderResourceBinding& a, const ShaderResourceBinding& b);

    // Get D3D12 visibility from shader type
    static D3D12_SHADER_VISIBILITY GetVisibilityFromShaderType(UINT shaderType);
};

} // namespace Rendering
