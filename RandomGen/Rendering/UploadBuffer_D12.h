#ifndef UPLOAD_BUFFER_H
#define UPLOAD_BUFFER_H

#include "RenderDefs.h"

#include <wrl.h>
#include <d3d12.h>

#include <memory>
#include <deque>

class UploadBuffer_D12
{
public:
		struct Allocation {
			void* cpu_loc;
			D3D12_GPU_VIRTUAL_ADDRESS gpu_loc;
		};

		explicit UploadBuffer_D12(size_t pageSize = _2MB);
		size_t GetPageSize() const {
			return m_pageSize;
		}
		Allocation Allocate(size_t sizeInBytes, size_t alignment);
		void Reset();

private:
	struct Page
	{
		Page(size_t sizeInBytes);
		~Page();
		bool HasSpace(size_t sizeInBytes, size_t alignment) const;
		// Allocate memory from the page.
		// Throws std::bad_alloc if the the allocation size is larger
		// that the page size or the size of the allocation exceeds the 
		// remaining space in the page.
		Allocation Allocate(size_t sizeInBytes, size_t alignment);
		// Reset the page for reuse.
		void Reset();
	private:

		Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

		// Base pointer.
		void* m_cpuData;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuData;

		// Allocated page size.
		size_t m_pageSize;
		// Current allocation offset in bytes.
		size_t m_offset;
	};

	using PagePool = std::deque<std::shared_ptr<Page>>;
	std::shared_ptr<Page> RequestPage();

	//
	PagePool m_pagePool;
	PagePool m_availablePages;
	std::shared_ptr<Page> m_currentPage;

	size_t m_pageSize;
};

#endif