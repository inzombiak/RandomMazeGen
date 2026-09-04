#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <vector>
#include <string>

// Need d3dcommon.h and d3dcompiler.h for shader reflection
#include <d3dcommon.h>
#include <d3dcompiler.h>

using Microsoft::WRL::ComPtr;

namespace Rendering {

// Describes a single resource binding in a shader
struct ShaderResourceBinding {
    std::string name = "";                    // Resource name (e.g., "ModelSB")
    D3D_SHADER_INPUT_TYPE type;          // CBV, SRV, UAV, Sampler
    UINT bindPoint;                      // Register slot (e.g., t0, b0, s0)
    UINT bindCount;                      // Array size (1 for non-arrays)
    UINT space;                          // Register space
    D3D12_SHADER_VISIBILITY visibility;  // Vertex, Pixel, or All
};

// Describes an input element (vertex attribute)
struct ShaderInputElement {
    std::string semanticName;            // e.g., "POSITION", "TEXCOORD"
    UINT semanticIndex;                  // Index (e.g., TEXCOORD0 = 0)
    DXGI_FORMAT format;                  // Data format
    UINT inputSlot;                      // Vertex buffer slot
    UINT alignedByteOffset;              // Offset in vertex structure
};

// Describes a constant buffer and its variables
struct ShaderConstantBuffer {
    std::string name;                    // CB name
    UINT bindPoint;                      // Register (b#)
    UINT size;                           // Size in bytes
    UINT variableCount;                  // Number of variables inside
};

struct ShaderRenderDefs {
    D3D12_CULL_MODE cullMode = D3D12_CULL_MODE::D3D12_CULL_MODE_BACK;

    // Declared in the shader as `Topology: Line` (default Triangle); the debug
    // line renderer needs a line-list PSO.
    D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    std::vector<DXGI_FORMAT> renderTargets;
};

// Complete metadata for a single shader stage
class ShaderMetadata {
public:
    ShaderMetadata() : shaderType(D3D12_SHVER_PIXEL_SHADER) {}

    std::vector<ShaderInputElement> inputElements;
    std::vector<ShaderResourceBinding> resources;
    std::vector<ShaderConstantBuffer> constantBuffers;

    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

    ShaderRenderDefs renderDefs;
    ComPtr<ID3DBlob> shaderBlob;

    UINT shaderType;  // Use UINT to store D3D12_SHVER_* values (Vertex, Pixel, etc.)
};

// Main reflection class - parses shader bytecode
class ShaderReflector {
public:
    ShaderReflector() = default;

    // Reflect a compiled shader blob and extract metadata
    bool ReflectShader(const std::wstring& shaderName, ShaderMetadata& outMetadata);

    // Helper: Convert D3D shader input type to string for debugging
    static const char* GetResourceTypeName(D3D_SHADER_INPUT_TYPE type);

    // Helper: Convert D3D component type + mask to DXGI format
    static DXGI_FORMAT GetInputElementFormat(D3D_REGISTER_COMPONENT_TYPE componentType,
                                              BYTE mask);

private:
    // Extract resource bindings (CBV, SRV, UAV, Samplers)
    void ReflectResources(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata);

    // Extract input signature (vertex attributes)
    void ReflectInputSignature(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata);

    // Extract constant buffer layouts
    void ReflectConstantBuffers(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata);

    void ReflectDecoratorComments(const std::wstring& shaderName, ShaderMetadata& outMetadata);

    // Determine shader visibility from shader type
    D3D12_SHADER_VISIBILITY GetVisibilityFromShaderType(UINT shaderType);
};

} // namespace Rendering
