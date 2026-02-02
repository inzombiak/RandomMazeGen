#include "RootSignatureBuilder.h"
#include <algorithm>
#include <iostream>
#include <print>
#include <sstream>

namespace Rendering {

void RootSignatureBuilder::AddShaderStage(const ShaderMetadata& metadata) {
    char debugMsg[256];
    sprintf_s(debugMsg, "AddShaderStage: Adding %d resources and %d constant buffers\n",
              (int)metadata.resources.size(), (int)metadata.constantBuffers.size());
    OutputDebugStringA(debugMsg);

    // Add all resource bindings from this shader stage
    for (const auto& binding : metadata.resources) {
        MergeResourceBinding(binding);
    }

    // Add constant buffers as bindings
    for (const auto& cb : metadata.constantBuffers) {
        ShaderResourceBinding cbBinding;
        cbBinding.name = cb.name;
        cbBinding.type = D3D_SIT_CBUFFER;
        cbBinding.bindPoint = cb.bindPoint;
        // Store size in DWORDs (32-bit values) in bindCount for InitAsConstants
        cbBinding.bindCount = (cb.size + 3) / 4; // Convert bytes to DWORDs, round up
        cbBinding.space = 0;
        cbBinding.visibility = GetVisibilityFromShaderType(metadata.shaderType);

        sprintf_s(debugMsg, "  Adding CB: name='%s', bindPoint=%d, size=%d DWORDs\n",
                  cb.name.c_str(), cb.bindPoint, cbBinding.bindCount);
        OutputDebugStringA(debugMsg);

        MergeResourceBinding(cbBinding);
    }

    sprintf_s(debugMsg, "AddShaderStage: Total bindings after merge: %d\n", (int)m_allBindings.size());
    OutputDebugStringA(debugMsg);
}

void RootSignatureBuilder::AddStaticSampler(const D3D12_STATIC_SAMPLER_DESC& sampler) {
    m_staticSamplers.push_back(sampler);
}

void RootSignatureBuilder::MergeResourceBinding(const ShaderResourceBinding& binding) {
    // Check if we already have this binding from another shader stage
    auto it = std::find_if(m_allBindings.begin(), m_allBindings.end(),
        [&binding](const ShaderResourceBinding& existing) {
            return existing.type == binding.type &&
                   existing.bindPoint == binding.bindPoint &&
                   existing.space == binding.space;
        });

    if (it != m_allBindings.end()) {
        // Merge visibility (e.g., VS + PS = ALL)
        it->visibility = CombineVisibility(it->visibility, binding.visibility);
    } else {
        // New binding
        m_allBindings.push_back(binding);
    }
}

bool RootSignatureBuilder::Build(D3D12_ROOT_SIGNATURE_DESC1& outDesc) {
    char debugMsg[256];
    sprintf_s(debugMsg, "Build: Starting with %d bindings\n", (int)m_allBindings.size());
    OutputDebugStringA(debugMsg);

    // Clear any previous build
    m_rootParameters.clear();
    m_resourceMappings.clear();

    // Sort bindings by type and bind point for better grouping
    std::sort(m_allBindings.begin(), m_allBindings.end(),
        [](const ShaderResourceBinding& a, const ShaderResourceBinding& b) {
            if (a.type != b.type) return a.type < b.type;
            return a.bindPoint < b.bindPoint;
        });

    // Create root parameters for each binding
    // Strategy: Create individual root descriptors for CBVs and single SRVs/UAVs
    //           Group consecutive textures into descriptor tables

    for (size_t i = 0; i < m_allBindings.size(); ++i) {
        const auto& binding = m_allBindings[i];

        sprintf_s(debugMsg, "  Processing binding[%d]: type=%d, name='%s', bindPoint=%d\n",
                  (int)i, binding.type, binding.name.c_str(), binding.bindPoint);
        OutputDebugStringA(debugMsg);

        // Check if this is part of a texture group that should be in a descriptor table
        if (binding.type == D3D_SIT_TEXTURE) {
            // Find consecutive textures with same visibility
            size_t tableStart = i;
            size_t tableEnd = i;

            while (tableEnd + 1 < m_allBindings.size() &&
                   m_allBindings[tableEnd + 1].type == D3D_SIT_TEXTURE &&
                   m_allBindings[tableEnd + 1].bindPoint == m_allBindings[tableEnd].bindPoint + 1 &&
                   m_allBindings[tableEnd + 1].visibility == binding.visibility) {
                tableEnd++;
            }

            // If we have multiple consecutive textures, create a descriptor table
            if (tableEnd > tableStart) {
                RootParameterInfo paramInfo;
                paramInfo.combinedVisibility = binding.visibility;

                // Create descriptor range for all textures in this group
                UINT numTextures = static_cast<UINT>(tableEnd - tableStart + 1);
                CD3DX12_DESCRIPTOR_RANGE1 range;
                range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                          numTextures,
                          binding.bindPoint,
                          binding.space,
                          D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

                paramInfo.ranges.push_back(range);
                // Add mappings for all textures in this table
                UINT rootParamIndex = static_cast<UINT>(m_rootParameters.size());
                for (size_t j = tableStart; j <= tableEnd; ++j) {
                    paramInfo.sourceBindings.push_back(m_allBindings[j]);

                    ResourceMapping mapping;
                    mapping.resourceName = m_allBindings[j].name;
                    mapping.rootParameterIndex = rootParamIndex;
                    mapping.bindPoint = m_allBindings[j].bindPoint;
                    mapping.type = m_allBindings[j].type;
                    m_resourceMappings.push_back(mapping);
                }
                m_rootParameters.push_back(paramInfo);
                m_rootParameters.back().parameter.InitAsDescriptorTable(1, &m_rootParameters.back().ranges[0], binding.visibility);
                i = tableEnd; // Skip the textures we just processed
                continue;
            }
        }

        // For non-grouped resources, create individual root descriptors
        RootParameterInfo paramInfo;
        paramInfo.combinedVisibility = binding.visibility;
        paramInfo.sourceBindings.push_back(binding);

        if (binding.type == D3D_SIT_CBUFFER) {
            // Check if name ends with "_Const" to determine if we should use root constants
            bool useRootConstants = false;
            if (binding.name.length() >= 6) {
                std::string suffix = binding.name.substr(binding.name.length() - 6);
                useRootConstants = (suffix == "_Const");
            }

            if (useRootConstants) {
                // Use root constants (inline data in root signature)
                // bindCount contains size in DWORDs
                paramInfo.parameter.InitAsConstants(
                    binding.bindCount,     // Num32BitValues (size in DWORDs)
                    binding.bindPoint,     // ShaderRegister (b#)
                    binding.space,         // RegisterSpace
                    binding.visibility);   // Visibility

                sprintf_s(debugMsg, "    -> Using ROOT CONSTANTS for '%s'\n", binding.name.c_str());
                OutputDebugStringA(debugMsg);
            } else {
                // Use constant buffer view (root descriptor pointer)
                paramInfo.parameter.InitAsConstantBufferView(
                    binding.bindPoint,
                    binding.space,
                    D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                    binding.visibility);

                sprintf_s(debugMsg, "    -> Using CONSTANT BUFFER VIEW for '%s'\n", binding.name.c_str());
                OutputDebugStringA(debugMsg);
            }
        }
        else if (binding.type == D3D_SIT_STRUCTURED || binding.type == D3D_SIT_BYTEADDRESS) {
            // Structured buffer or byte address buffer - use root SRV descriptor
            paramInfo.parameter.InitAsShaderResourceView(
                binding.bindPoint,
                binding.space,
                D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                binding.visibility);
        }
        else if (binding.type == D3D_SIT_TEXTURE) {
            // Single texture - create a descriptor table with one entry
            CD3DX12_DESCRIPTOR_RANGE1 range;
            range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                      1,
                      binding.bindPoint,
                      binding.space,
                      D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);

            paramInfo.ranges.push_back(range);
            paramInfo.parameter.InitAsDescriptorTable(1, &paramInfo.ranges[0], binding.visibility);
        }
        else if (binding.type == D3D_SIT_UAV_RWSTRUCTURED || binding.type == D3D_SIT_UAV_RWBYTEADDRESS) {
            // UAV - use root UAV descriptor
            paramInfo.parameter.InitAsUnorderedAccessView(
                binding.bindPoint,
                binding.space,
                D3D12_ROOT_DESCRIPTOR_FLAG_NONE,
                binding.visibility);
        }
        else if (binding.type == D3D_SIT_SAMPLER) {
            // Samplers should be static samplers, not root parameters
            // Skip them here - they should be added via AddStaticSampler
            continue;
        }
        else {
            // Unsupported type - skip
            continue;
        }

        // Add resource mapping
        ResourceMapping mapping;
        mapping.resourceName = binding.name;
        mapping.rootParameterIndex = static_cast<UINT>(m_rootParameters.size());
        mapping.bindPoint = binding.bindPoint;
        mapping.type = binding.type;
        m_resourceMappings.push_back(mapping);

        m_rootParameters.push_back(paramInfo);

        sprintf_s(debugMsg, "    -> Created root parameter #%d\n", (int)(m_rootParameters.size() - 1));
        OutputDebugStringA(debugMsg);
    }

    sprintf_s(debugMsg, "Build: Created %d root parameters total\n", (int)m_rootParameters.size());
    OutputDebugStringA(debugMsg);

    // Build the root parameters array (store as member to keep pointers valid)
    m_builtRootParams.clear();
    for (auto& paramInfo : m_rootParameters) {
        m_builtRootParams.push_back(paramInfo.parameter);
    }

    // Create root signature description (return this for RootSignature_D12 constructor)
    // The pointers will remain valid as long as the builder exists
    outDesc.NumParameters = static_cast<UINT>(m_builtRootParams.size());
    outDesc.pParameters = m_builtRootParams.data();
    outDesc.NumStaticSamplers = static_cast<UINT>(m_staticSamplers.size());
    outDesc.pStaticSamplers = m_staticSamplers.data();
    outDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    return true;
}

int RootSignatureBuilder::GetRootParameterIndex(const std::string& resourceName) const {
    auto it = std::find_if(m_resourceMappings.begin(), m_resourceMappings.end(),
        [&resourceName](const ResourceMapping& mapping) {
            return mapping.resourceName == resourceName;
        });

    return (it != m_resourceMappings.end()) ? it->rootParameterIndex : -1;
}

int RootSignatureBuilder::GetRootParameterIndexByBindPoint(D3D_SHADER_INPUT_TYPE type, UINT bindPoint) const {
    auto it = std::find_if(m_resourceMappings.begin(), m_resourceMappings.end(),
        [type, bindPoint](const ResourceMapping& mapping) {
            return mapping.type == type && mapping.bindPoint == bindPoint;
        });

    return (it != m_resourceMappings.end()) ? it->rootParameterIndex : -1;
}

void RootSignatureBuilder::PrintLayout() const {
    std::stringstream ss;
    ss << "\n=== Root Signature Layout ===\n";

    for (size_t i = 0; i < m_rootParameters.size(); ++i) {
        const auto& paramInfo = m_rootParameters[i];
        ss << "  [" << i << "] ";

        // Determine parameter type
        if (paramInfo.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS) {
            ss << "Root Constants (b" << paramInfo.sourceBindings[0].bindPoint << ") - "
               << paramInfo.sourceBindings[0].name
               << " [" << paramInfo.sourceBindings[0].bindCount << " DWORDs]";
        }
        else if (paramInfo.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_CBV) {
            ss << "CBV (b" << paramInfo.sourceBindings[0].bindPoint << ") - "
               << paramInfo.sourceBindings[0].name;
        }
        else if (paramInfo.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_SRV) {
            ss << "SRV (t" << paramInfo.sourceBindings[0].bindPoint << ") - "
               << paramInfo.sourceBindings[0].name;
        }
        else if (paramInfo.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_UAV) {
            ss << "UAV (u" << paramInfo.sourceBindings[0].bindPoint << ") - "
               << paramInfo.sourceBindings[0].name;
        }
        else if (paramInfo.parameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
            ss << "Descriptor Table";
            if (paramInfo.sourceBindings.size() > 1) {
                ss << " (t" << paramInfo.sourceBindings[0].bindPoint
                   << "-t" << paramInfo.sourceBindings.back().bindPoint << ")";
            } else {
                ss << " (t" << paramInfo.sourceBindings[0].bindPoint << ")";
            }
            ss << ":";
            for (const auto& binding : paramInfo.sourceBindings) {
                ss << "\n      - " << binding.name;
            }
        }

        // Add visibility
        ss << " (Visibility: ";
        switch (paramInfo.combinedVisibility) {
            case D3D12_SHADER_VISIBILITY_VERTEX:   ss << "VERTEX"; break;
            case D3D12_SHADER_VISIBILITY_PIXEL:    ss << "PIXEL"; break;
            case D3D12_SHADER_VISIBILITY_GEOMETRY: ss << "GEOMETRY"; break;
            case D3D12_SHADER_VISIBILITY_ALL:      ss << "ALL"; break;
            default: ss << "OTHER"; break;
        }
        ss << ")\n";
    }

    ss << "\nStatic Samplers:\n";
    for (size_t i = 0; i < m_staticSamplers.size(); ++i) {
        ss << "  [s" << m_staticSamplers[i].ShaderRegister << "] ";
        if (m_staticSamplers[i].Filter == D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT ||
            m_staticSamplers[i].Filter == D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR) {
            ss << "Comparison sampler";
        } else {
            ss << "Standard sampler";
        }
        ss << "\n";
    }

    ss << "=============================\n";

    OutputDebugStringA(ss.str().c_str());
    std::cout << ss.str() << std::endl;
}

D3D12_SHADER_VISIBILITY RootSignatureBuilder::CombineVisibility(D3D12_SHADER_VISIBILITY a, D3D12_SHADER_VISIBILITY b) {
    if (a == b) return a;
    return D3D12_SHADER_VISIBILITY_ALL;
}

bool RootSignatureBuilder::CanMergeBindings(const ShaderResourceBinding& a, const ShaderResourceBinding& b) {
    return a.type == b.type &&
           a.bindPoint == b.bindPoint &&
           a.space == b.space;
}

D3D12_SHADER_VISIBILITY RootSignatureBuilder::GetVisibilityFromShaderType(UINT shaderType) {
    switch (shaderType) {
        case D3D12_SHVER_VERTEX_SHADER:   return D3D12_SHADER_VISIBILITY_VERTEX;
        case D3D12_SHVER_PIXEL_SHADER:    return D3D12_SHADER_VISIBILITY_PIXEL;
        case D3D12_SHVER_GEOMETRY_SHADER: return D3D12_SHADER_VISIBILITY_GEOMETRY;
        case D3D12_SHVER_HULL_SHADER:     return D3D12_SHADER_VISIBILITY_HULL;
        case D3D12_SHVER_DOMAIN_SHADER:   return D3D12_SHADER_VISIBILITY_DOMAIN;
        default:                          return D3D12_SHADER_VISIBILITY_ALL;
    }
}

} // namespace Rendering
