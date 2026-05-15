#ifndef RENDERER_D12_H
#define RENDERER_D12_H

// DirectX 12 specific headers.
#include <dxgi1_6.h>
#include <d3dcompiler.h>

// GLM Math library
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// D3D12 extension library.
#include "d3dx12/d3dx12.h"


// Windows Runtime Library. Needed for Microsoft::WRL::ComPtr<> template class.
#include <wrl.h>
using namespace Microsoft::WRL;

#include "CommandQueue_D12.h"
#include "MazeGenDefs.h"
#include "RenderDefs.h"
#include "ShaderReflection.h"
#include "RootSignatureBuilder.h"

#include "Texture_D12.h"
#include "../Renderable.h"

struct PerEntityData
{
	glm::mat4 M;
	unsigned int data;
};

struct SceneData
{
	glm::mat4 camVP;
	glm::mat4 sunVP;
	glm::vec4 sunPos;
	glm::vec4 camPos;
	glm::vec2 invShadowTexSize;
	glm::vec2 _pad0;           
	glm::vec4 _pad1[5];       
};

class Tile;
class UploadBuffer_D12;
class DescriptorAllocator_D12;
class DescriptorAllocation_D12;
class DynamicDescriptorHeap_D12;
class RootSignature_D12;
class Texture_D12;

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

#include "../Material.h"

struct Buffer {
	ComPtr<ID3D12Resource> m_resource;
	D3D12_VERTEX_BUFFER_VIEW m_bufferView;
	SRVAllocation m_srvAlloc;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_GPUHandle;
};

struct MaterialBatch {
	std::shared_ptr<Material> material;
	uint32_t startInstanceOffset;
	uint32_t instanceCount;
};

//@ZGTODO merge this with the decriptor alocator
class Tile;
struct ExampleDescriptorHeapAllocator
{
	ID3D12DescriptorHeap* Heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
	D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
	D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
	UINT                        HeapHandleIncrement;
	ImVector<int>               FreeIndices;

	void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	{
		Heap = heap;
		D3D12_DESCRIPTOR_HEAP_DESC desc = heap->GetDesc();
		HeapType = desc.Type;
		HeapStartCpu = Heap->GetCPUDescriptorHandleForHeapStart();
		HeapStartGpu = Heap->GetGPUDescriptorHandleForHeapStart();
		HeapHandleIncrement = device->GetDescriptorHandleIncrementSize(HeapType);
		FreeIndices.reserve((int)desc.NumDescriptors);
		for (int n = desc.NumDescriptors; n > 0; n--)
			FreeIndices.push_back(n);
	}
	void Destroy()
	{
		Heap = nullptr;
		FreeIndices.clear();
	}
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle)
	{
		// Validate that we have descriptors available
		if (FreeIndices.empty())
		{
			throw std::runtime_error("ImGui descriptor heap is full. Increase IMGUI_HEAP_SIZE or fix descriptor leak.");
		}

		int idx = FreeIndices.back();
		FreeIndices.pop_back();

		// Validate index is within heap bounds
		assert(idx >= 0 && idx < (int)Heap->GetDesc().NumDescriptors && "ImGui descriptor index out of bounds");

		out_cpu_desc_handle->ptr = HeapStartCpu.ptr + (idx * HeapHandleIncrement);
		out_gpu_desc_handle->ptr = HeapStartGpu.ptr + (idx * HeapHandleIncrement);
	}
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle)
	{
		int cpu_idx = (int)((out_cpu_desc_handle.ptr - HeapStartCpu.ptr) / HeapHandleIncrement);
		int gpu_idx = (int)((out_gpu_desc_handle.ptr - HeapStartGpu.ptr) / HeapHandleIncrement);

		// Validate that CPU and GPU indices match (they should always be the same)
		assert(cpu_idx == gpu_idx && "ImGui CPU and GPU descriptor indices don't match");

		// Validate index is within heap bounds
		assert(cpu_idx >= 0 && cpu_idx < (int)Heap->GetDesc().NumDescriptors && "ImGui descriptor index out of bounds");

		FreeIndices.push_back(cpu_idx);
	}
};


class Renderer_D12 {

	public:
		Renderer_D12();
		~Renderer_D12();
		void PostInit();
		void ResizeTargets();
		void UpdateRenderTargetViews();
		void Shadowmap();
		void Render();
		void Shutdown();
		bool IsInitialized() const;

		void PopulateVertexBuffer(const VertexInput *data, size_t count);
		void PopulateIndexBuffer(const unsigned int *data, size_t count);
		std::shared_ptr<Material> CreateMaterial(const std::string name, const std::wstring& vertexShaderName, const std::wstring& pixelShaderName, const std::vector<std::wstring>& textures = {});
		std::shared_ptr<Material> GetMaterial(const std::string& name) const;
		void UpdateInstanceData(const std::vector<Renderable>& renderables);
		void LoadTextures();
		// Resize the depth buffer to match the size of the client area.
		void ResizeDepthBuffer(int width, int height);

		void UpdateMVP(float fov, glm::vec3 camPos, glm::vec3 camFwd, glm::vec3 camRight, glm::vec3 camUp, glm::vec4 sunPos);

		ComPtr<ID3D12Device2> GetDevice() const;
		uint64_t GetCurrentFrameCount() const;
		uint32_t GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		D3D_ROOT_SIGNATURE_VERSION GetHighestRootSigVer() const;

		bool GUIInitialized() const;

		ExampleDescriptorHeapAllocator				m_imGUIAllocator;

	private:

		PipelineStateObject* BuildPipelineState(const std::wstring& vertexShaderName, const std::wstring& pixelShaderName);
		std::shared_ptr<Texture_D12> MakeOrGetTexture(const std::wstring& name, const std::wstring& filepath, std::map<size_t, std::shared_ptr<Texture_D12>>& resourceMap, std::shared_ptr<CommandList_D12> cmdList = nullptr);
		std::shared_ptr<Buffer> CreateSRVBuffer(const std::string& name, size_t size, size_t count, void* data, std::shared_ptr<CommandList_D12> cmdList);
		std::shared_ptr<Buffer> CreateCBVBuffer(const std::string& name, size_t size, void* data, std::shared_ptr<CommandList_D12> cmdList);

		SRVAllocation GetNextSRVAlloc();

		D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;

		Renderer_D12(const Renderer_D12&) = delete;
		Renderer_D12& operator=(const Renderer_D12&) = delete;

		static const uint8_t NUM_BACKBUFFER_FRAMES = 3;

		UINT		m_currentBufferIdx;

		ComPtr<ID3D12Device2>				m_device;
		std::shared_ptr<CommandQueue_D12>	m_commQueue;
		ComPtr<IDXGISwapChain4>				m_swapChain;
		ComPtr<ID3D12Resource>				m_backbuffers[NUM_BACKBUFFER_FRAMES];
		ComPtr<ID3D12Resource>				m_depthBuffer;

		D3D_ROOT_SIGNATURE_VERSION			m_highestRootSignatureVersion;

		std::shared_ptr<DescriptorAllocator_D12>  m_rtvAllocator;
		std::shared_ptr<DescriptorAllocation_D12> m_rtvs;
		std::shared_ptr<DescriptorAllocator_D12>  m_dsvAllocator;
		std::shared_ptr<DescriptorAllocation_D12> m_dsvs;

		std::shared_ptr<DescriptorAllocator_D12>	m_shaderResourceAllocator;
		std::shared_ptr<DynamicDescriptorHeap_D12>	m_shaderResourceDynHeap;
		std::vector<SRVAllocationPage>				m_srvAllocPages;

		const UINT						IMGUI_HEAP_SIZE = 64;
		bool							m_imGUIInitalized = false;
		ComPtr<ID3D12DescriptorHeap>	m_imGUISRVHeap;


		uint64_t m_numInstances = 0;

		ComPtr<ID3D12Resource> m_vertexBuffer;
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		ComPtr<ID3D12Resource> m_indexBuffer;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

		std::shared_ptr<Buffer> m_perEntityDataBuffer;
		std::shared_ptr<Buffer> m_sceneDataBuffer;
		std::vector<MaterialBatch> m_materialBatches;

		// Pipeline state object.
		std::shared_ptr<Texture_D12> m_shadowTexture;

		std::map<size_t, std::shared_ptr<Material>> m_materialMap;
		std::map<size_t, PipelineStateObject> m_psoMap;
		std::map<size_t, std::shared_ptr<Texture_D12>> m_textureMap;

		size_t m_basicLitMatId;
		size_t m_shadowmapMatId;

		D3D12_VIEWPORT m_viewport;
		D3D12_RECT m_scissorRect;

		size_t m_indexCount;
		int m_worldWidth;
		UINT8* m_sceneDataBegin;
		SceneData m_sceneData;

		//Fencing
		uint64_t			m_fenceValue = 0;
		uint64_t			m_shadowMapFenceVal = 0;
		uint64_t			m_perFrameFenceValues[NUM_BACKBUFFER_FRAMES] = {};

		uint64_t			m_currentFrame = 0;

		bool m_tearingSupported = false;
		bool m_initalized = false;
};

#endif