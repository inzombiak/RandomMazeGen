#include "ShaderReflection.h"
#include "DX12_Helpers.h"
#include "RenderDefs.h"
#include <cassert>
#include <fstream>
#include <string>

namespace Rendering {

bool ShaderReflector::ReflectShader(const std::wstring& shaderName, ShaderMetadata& outMetadata) {

    ThrowIfFailed(D3DReadFileToBlob((shaderName + L".cso").data(), &outMetadata.shaderBlob));
    
    if (!outMetadata.shaderBlob) return false;

    // Create shader reflection interface
    ComPtr<ID3D12ShaderReflection> reflection;
    HRESULT hr = D3DReflect(
        outMetadata.shaderBlob->GetBufferPointer(),
        outMetadata.shaderBlob->GetBufferSize(),
        IID_PPV_ARGS(&reflection)
    );

    if (FAILED(hr)) return false;

    // Get shader description
    D3D12_SHADER_DESC shaderDesc = {};
    reflection->GetDesc(&shaderDesc);

    outMetadata.shaderType = D3D12_SHVER_GET_TYPE(shaderDesc.Version);

    // Extract all metadata
    ReflectResources(reflection.Get(), outMetadata);
    ReflectInputSignature(reflection.Get(), outMetadata);
    ReflectConstantBuffers(reflection.Get(), outMetadata);

    ReflectDecoratorComments(shaderName, outMetadata);

    return true;
}

void ShaderReflector::ReflectResources(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata) {
    D3D12_SHADER_DESC shaderDesc = {};
    reflection->GetDesc(&shaderDesc);

    // Iterate through all bound resources
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
        reflection->GetResourceBindingDesc(i, &bindDesc);

        // Skip constant buffers - they're handled separately in ReflectConstantBuffers
        // because we need size information for InitAsConstants
        if (bindDesc.Type == D3D_SIT_CBUFFER) {
            continue;
        }

        ShaderResourceBinding binding;
        binding.name = bindDesc.Name;
        binding.type = bindDesc.Type;
        binding.bindPoint = bindDesc.BindPoint;
        binding.bindCount = bindDesc.BindCount;
        binding.space = bindDesc.Space;
        binding.visibility = GetVisibilityFromShaderType(metadata.shaderType);

        metadata.resources.push_back(binding);
    }
}

void ShaderReflector::ReflectInputSignature(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata) {
    D3D12_SHADER_DESC shaderDesc = {};
    reflection->GetDesc(&shaderDesc);

    // Only vertex shaders have input signatures we care about
    if (metadata.shaderType != D3D12_SHVER_VERTEX_SHADER) return;

    for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
        D3D12_SIGNATURE_PARAMETER_DESC paramDesc = {};
        reflection->GetInputParameterDesc(i, &paramDesc);

        // Skip system values (SV_InstanceID, etc.)
        if (paramDesc.SystemValueType != D3D_NAME_UNDEFINED) continue;

        ShaderInputElement element;
        element.semanticName = paramDesc.SemanticName;
        element.semanticIndex = paramDesc.SemanticIndex;
        element.format = GetInputElementFormat(paramDesc.ComponentType, paramDesc.Mask);
        element.inputSlot = 0; // Default to slot 0, can be customized
        element.alignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;

        metadata.inputElements.push_back(element);
    }
}

void ShaderReflector::ReflectConstantBuffers(ID3D12ShaderReflection* reflection, ShaderMetadata& metadata) {
    D3D12_SHADER_DESC shaderDesc = {};
    reflection->GetDesc(&shaderDesc);

    // Debug: Print how many constant buffers we found
    char debugMsg[256];
    sprintf_s(debugMsg, "ReflectConstantBuffers: Found %d constant buffers, %d bound resources\n",
              shaderDesc.ConstantBuffers, shaderDesc.BoundResources);
    OutputDebugStringA(debugMsg);

    for (UINT i = 0; i < shaderDesc.ConstantBuffers; ++i) {
        ID3D12ShaderReflectionConstantBuffer* cbReflection = reflection->GetConstantBufferByIndex(i);

        D3D12_SHADER_BUFFER_DESC bufferDesc = {};
        cbReflection->GetDesc(&bufferDesc);

        sprintf_s(debugMsg, "  CB[%d]: Name='%s', Size=%d bytes\n", i, bufferDesc.Name, bufferDesc.Size);
        OutputDebugStringA(debugMsg);

        // Find the binding point for this CB
        D3D12_SHADER_INPUT_BIND_DESC bindDesc = {};
        bool found = false;
        for (UINT j = 0; j < shaderDesc.BoundResources; ++j) {
            reflection->GetResourceBindingDesc(j, &bindDesc);

            if (bindDesc.Type == D3D_SIT_CBUFFER && strcmp(bindDesc.Name, bufferDesc.Name) == 0) {
                found = true;
                sprintf_s(debugMsg, "    -> MATCHED at bind point b%d\n", bindDesc.BindPoint);
                OutputDebugStringA(debugMsg);
                break;
            }
        }

        if (!found) {
            OutputDebugStringA("    -> NOT FOUND in bound resources, skipping\n");
            continue;
        }

        ShaderConstantBuffer cb;
        cb.name = bufferDesc.Name;
        cb.bindPoint = bindDesc.BindPoint;
        cb.size = bufferDesc.Size;
        cb.variableCount = bufferDesc.Variables;

        metadata.constantBuffers.push_back(cb);
    }
}

std::string GetValueInLine(std::string& line, const std::string& varName = "", char delim = ':') {
    line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
    if (varName.size() > 0) {
        auto namePos = line.find(varName);
        if (namePos == std::string::npos)
            return "";

        line = line.substr(namePos +  varName.size(), line.size() - namePos);
    }
    line.erase(std::remove(line.begin(), line.end(), delim), line.end());

    return line;
}

void ShaderReflector::ReflectDecoratorComments(const std::wstring& shaderName, ShaderMetadata& outMetadata) {
    std::ifstream fileInput;
    std::string line;
    std::wstring filepath = L_SHADER_PATH + shaderName + L".hlsl";
    // open file to search
    fileInput.open(filepath.c_str());
    std::string val;
    if (fileInput.is_open()) {
        while (!fileInput.eof()) {
            getline(fileInput, line);
            if (line.starts_with("/*~")) {
                //We have a decorator
                getline(fileInput, line);
                int varNameIdx = std::string::npos;
                if (line.find("StaticSampler") != std::string::npos) {
                    D3D12_STATIC_SAMPLER_DESC sampler;

                    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
                    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
                    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
                    sampler.MipLODBias = 0;
                    sampler.MaxAnisotropy = 0;
                    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
                    sampler.MinLOD = 0.0f;
                    sampler.MaxLOD = D3D12_FLOAT32_MAX;
                    sampler.ShaderRegister = 0;
                    sampler.RegisterSpace = 0;
                    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

                    getline(fileInput, line);
                    auto start = line.find("{");
                    if (start != std::string::npos) {
                        getline(fileInput, line);
                        while (line.find("}") == std::string::npos) {
                            if (val = GetValueInLine(line, "Register"); val.size() > 0) {
                                sampler.ShaderRegister = std::stoi(val);
                            }
                            else if (val = GetValueInLine(line, "Filter"); val.size() > 0) {
                                if (val.compare("CompMinMagLinearMipPoint") == 0) {
                                    sampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
                                }
                            }
                            else if (val = GetValueInLine(line, "CompFunc"); val.size() > 0) {
                                if (val.compare("Less") == 0) {
                                    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS;
                                }
                            }

                            getline(fileInput, line);
                        }
                    }

                    outMetadata.staticSamplers.push_back(sampler);
                }
                else if (val = GetValueInLine(line, "CullMode"); val.size() > 0) {
                    if (val.compare("Back") == 0) {
                        outMetadata.renderDefs.cullMode = D3D12_CULL_MODE_BACK;
                    }
                    else if (val.compare("Front") == 0) {
                        outMetadata.renderDefs.cullMode = D3D12_CULL_MODE_FRONT;
                    }
                    else if (val.compare("None") == 0) {
                        outMetadata.renderDefs.cullMode = D3D12_CULL_MODE_NONE;
                    }
                }
                else if (line.find("RenderTargets") != std::string::npos) {
                    getline(fileInput, line);
                    auto start = line.find("{");
                    if (start != std::string::npos) {
                        getline(fileInput, line);
                        while (line.find("}") == std::string::npos) {
                            val = GetValueInLine(line);
                            if (val.compare("RGBA8_UNORM") == 0) {
                                outMetadata.renderDefs.renderTargets.push_back(DXGI_FORMAT_R8G8B8A8_UNORM);
                            }
                            getline(fileInput, line);
                        }
                    }
                }
            }
        }
        fileInput.close();
    }
}

D3D12_SHADER_VISIBILITY ShaderReflector::GetVisibilityFromShaderType(UINT shaderType) {
    switch (shaderType) {
        case D3D12_SHVER_VERTEX_SHADER:   return D3D12_SHADER_VISIBILITY_VERTEX;
        case D3D12_SHVER_PIXEL_SHADER:    return D3D12_SHADER_VISIBILITY_PIXEL;
        case D3D12_SHVER_GEOMETRY_SHADER: return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case D3D12_SHVER_HULL_SHADER:     return D3D12_SHADER_VISIBILITY_HULL;
        case D3D12_SHVER_DOMAIN_SHADER:   return D3D12_SHADER_VISIBILITY_DOMAIN;
        default:                          return D3D12_SHADER_VISIBILITY_ALL;
    }
}

DXGI_FORMAT ShaderReflector::GetInputElementFormat(D3D_REGISTER_COMPONENT_TYPE componentType, BYTE mask) {
    // Count number of components (1-4) from mask
    UINT numComponents = 0;
    for (int i = 0; i < 4; ++i) {
        if (mask & (1 << i)) numComponents++;
    }

    // Map component type + count to DXGI format
    if (componentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        switch (numComponents) {
            case 1: return DXGI_FORMAT_R32_FLOAT;
            case 2: return DXGI_FORMAT_R32G32_FLOAT;
            case 3: return DXGI_FORMAT_R32G32B32_FLOAT;
            case 4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        }
    }
    else if (componentType == D3D_REGISTER_COMPONENT_UINT32) {
        switch (numComponents) {
            case 1: return DXGI_FORMAT_R32_UINT;
            case 2: return DXGI_FORMAT_R32G32_UINT;
            case 3: return DXGI_FORMAT_R32G32B32_UINT;
            case 4: return DXGI_FORMAT_R32G32B32A32_UINT;
        }
    }
    else if (componentType == D3D_REGISTER_COMPONENT_SINT32) {
        switch (numComponents) {
            case 1: return DXGI_FORMAT_R32_SINT;
            case 2: return DXGI_FORMAT_R32G32_SINT;
            case 3: return DXGI_FORMAT_R32G32B32_SINT;
            case 4: return DXGI_FORMAT_R32G32B32A32_SINT;
        }
    }

    return DXGI_FORMAT_UNKNOWN;
}

const char* ShaderReflector::GetResourceTypeName(D3D_SHADER_INPUT_TYPE type) {
    switch (type) {
        case D3D_SIT_CBUFFER:   return "ConstantBuffer";
        case D3D_SIT_TBUFFER:   return "TextureBuffer";
        case D3D_SIT_TEXTURE:   return "Texture";
        case D3D_SIT_SAMPLER:   return "Sampler";
        case D3D_SIT_UAV_RWTYPED: return "RWTexture";
        case D3D_SIT_STRUCTURED: return "StructuredBuffer";
        case D3D_SIT_UAV_RWSTRUCTURED: return "RWStructuredBuffer";
        case D3D_SIT_BYTEADDRESS: return "ByteAddressBuffer";
        case D3D_SIT_UAV_RWBYTEADDRESS: return "RWByteAddressBuffer";
        default: return "Unknown";
    }
}

} // namespace Rendering
