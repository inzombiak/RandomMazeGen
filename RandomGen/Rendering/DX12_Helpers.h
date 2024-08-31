#ifndef DX12_HELPERS_H
#define DX12_HELPERS_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // For HRESULT

// DirectX 12 specific headers.
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

// D3D12 extension library.
#include "d3dx12/d3dx12.h"


// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

// From DXSampleHelper.h 
// Source: https://github.com/Microsoft/DirectX-Graphics-Samples
inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) {
        throw std::exception();
    }

}


#endif