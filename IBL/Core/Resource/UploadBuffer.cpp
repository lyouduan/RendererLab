#include "UploadBuffer.h"
#include "GraphicsCore.h"

using namespace Graphics;

void UploadBuffer::Create(const std::wstring& Name, size_t BufferSize)
{
	Destroy();

	m_BufferSize = BufferSize;

	// create an upload heap, this is CPU-visible, but it's write combined memory,
	// so avoid reading back from it
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// Upload buffers must be 1-dimensional
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = m_BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, MY_IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(Name.c_str());
#endif
}

void* UploadBuffer::Map(void)
{
	void* Memory;
	auto range = CD3DX12_RANGE(0, m_BufferSize);
	m_pResource->Map(0, &range, &Memory);
	return Memory;
}

void UploadBuffer::Unmap(size_t begin, size_t end)
{
	auto range = CD3DX12_RANGE(begin, std::min(end, m_BufferSize));
	m_pResource->Unmap(0, &range);
}
