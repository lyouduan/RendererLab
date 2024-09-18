#pragma once
#include "pch.h"
#include "GraphicsCore.h"
#include "GpuResource.h"
#include "GpuBuffer.h"
#include "UploadBuffer.h"
#include "ColorBuffer.h"
#include "DepthBuffer.h"
#include "LinearAllocator.h"
#include "DynamicDescriptorHeap.h"
#include "CubeMapBuffer.h"

#include <queue>

class CommandContext;
class GraphicsContext;
class RootSignature;

class PSO;


struct DWParam
{
	DWParam(FLOAT f) : Float(f) {}
	DWParam(UINT u) : Uint(u) {}
	DWParam(INT i) : Int(i) {}

	void operator= (FLOAT f) { Float = f; }
	void operator= (UINT u) { Uint = u; }
	void operator= (INT i) { Int = i; }

	union
	{
		FLOAT Float;
		UINT Uint;
		INT Int;
	};
};

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

class ContextManager
{
public:

	ContextManager(void) {}

	CommandContext* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);

	void FreeContext(CommandContext*);
	void DestroyAllContexts();

private:

	std::vector<std::unique_ptr<CommandContext>> sm_ContextPool[4];
	std::queue<CommandContext*> sm_AvailableContexts[4];
	std::mutex sm_ContextAllocationMutex;
};

struct NonCopyable
{
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

class CommandContext : NonCopyable
{
	friend ContextManager;

private:
	CommandContext(D3D12_COMMAND_LIST_TYPE Type);

	void Reset(void);

public:
	~CommandContext(void);

	static void DestroyAllContexts(void);

	static CommandContext& Begin(const std::wstring ID = L"");

	// Flush existing commands to the GPU but keep the context alive
	uint64_t Flush(bool WaitForCompletion = false);

	// Flush existing commands and release the current context
	uint64_t Finish(bool WaitForCompletion = false);

	// Prepare to render by reserving a command list and command allocator
	void Initialize(void);

	GraphicsContext& GetGraphicsContext() {
		ASSERT(m_Type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
		return reinterpret_cast<GraphicsContext&>(*this);
	}

	ID3D12GraphicsCommandList* GetCommandList() {
		return m_CommandList;
	}

	void CopyBuffer(GpuResource& Dest, GpuResource& Src);
	void CopyBufferRegion(GpuResource& Dest, size_t DestOffset, GpuResource& Src, size_t SrcOffset, size_t NumBytes);
	void CopySubresource(GpuResource& Dest, UINT DestSubIndex, GpuResource& Src, UINT SrcSubIndex);

	DynAlloc ReserveUploadMemory(size_t SizeInBytes)
	{
		return m_CpuLinearAllocator.Allocate(SizeInBytes);
	}

	static void InitializeTexture(GpuResource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	static void InitializeBuffer(GpuBuffer& Dest, const void* Data, size_t NumBytes, size_t DestOffset = 0);
	static void InitializeBuffer(GpuBuffer& Dest, const UploadBuffer& Src, size_t SrcOffset, size_t NumBytes = -1, size_t DestOffset = 0);
	static void InitializeTextureArraySlice(GpuResource& Dest, UINT SliceIndex, GpuResource& Src);

	//void WriteBuffer(GpuResource& Dest, size_t DestOffset, const void* Data, size_t NumBytes);
	//void FillBuffer(GpuResource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

	void TransitionResource(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void BeginResourceTransition(GpuResource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);

	void FlushResourceBarriers(void);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr);
	void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[]);
	void SetPipelineState(const PSO& PSO);

	void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

protected:

	void BindDescriptorHeaps(void);

	CommandListManager* m_OwningManager;
	ID3D12GraphicsCommandList* m_CommandList;
	ID3D12CommandAllocator* m_CurrentAllocator;

	ID3D12RootSignature* m_CurGraphicsRootSignature;
	ID3D12RootSignature* m_CurComputeRootSignature;
	ID3D12PipelineState* m_CurPipelineState;

	DynamicDescriptorHeap m_DynamicViewDescriptorHeap;		// HEAP_TYPE_CBV_SRV_UAV
	DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;	// HEAP_TYPE_SAMPLER

	D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
	UINT m_NumBarriersToFlush;

	ID3D12DescriptorHeap* m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	LinearAllocator m_CpuLinearAllocator; // CpulinearAllocator is used to create upload buffer
	LinearAllocator m_GpuLinearAllocator; // GpulinearAllocator is used to create default buffer

	std::wstring m_ID;
	void SetID(const std::wstring& ID) { m_ID = ID; }

	D3D12_COMMAND_LIST_TYPE m_Type;
};

class GraphicsContext : public CommandContext
{
public:
	static GraphicsContext& Begin(const std::wstring& ID = L"")
	{
		return CommandContext::Begin(ID).GetGraphicsContext();
	}

	void ClearColor(ColorBuffer& Target, D3D12_RECT* Rect = nullptr);
	void ClearColor(CubeMapBuffer& Target, D3D12_RECT* Rect = nullptr);
	void ClearColor(ColorBuffer& Target, float Colour[4], D3D12_RECT* Rect = nullptr);
	void ClearDepth(DepthBuffer& Target);
	void ClearStencil(DepthBuffer& Target);
	void ClearDepthAndStencil(DepthBuffer& Target);

	void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

	void SetRootSignature(const RootSignature& RootSig);

	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV) { SetRenderTargets(1, &RTV); }
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(1, &RTV, DSV); }
	void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV) { SetRenderTargets(0, nullptr, DSV); }

	void SetViewport(const D3D12_VIEWPORT& vp);
	void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
	void SetScissor(const D3D12_RECT& rect);
	void SetScissor(UINT left, UINT top, UINT right, UINT bottom);
	void SetViewportAndScissor(const D3D12_VIEWPORT& vp, const D3D12_RECT& rect);
	void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h);

	void SetStencilRef(UINT StencilRef);
	void SetBlendFactor(Color BlendFactor);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

	void SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants);
	void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
	void SetConstants(UINT RootIndex, DWParam X);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
	void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
	void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void* BufferData);
	void SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset = 0);
	void SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset = 0);
	void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

	void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
	void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView);
	void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView);
	void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
	void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData);
	void SetDynamicIB(size_t IndexCount, const uint16_t* IBData);
	void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

	void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
	void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
		UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
	void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
		INT BaseVertexLocation, UINT StartInstanceLocation);

private:

};
inline void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
{
	DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
}

inline void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

inline void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
	UINT StartVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	//m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	//m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

inline void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
	INT BaseVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}
inline void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView)
{
	SetVertexBuffers(Slot, 1, &VBView);
}

inline void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
{
	m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
}

inline void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& IBView)
{
	m_CommandList->IASetIndexBuffer(&IBView);
}

inline void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
	m_CommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
}

inline void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}


inline void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}


inline void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}


inline void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}
inline void GraphicsContext::SetBufferUAV(UINT RootIndex, const GpuBuffer& UAV, UINT64 Offset)
{
	ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
	m_CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

inline void GraphicsContext::SetBufferSRV(UINT RootIndex, const GpuBuffer& SRV, UINT64 Offset)
{
	ASSERT((SRV.m_UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
	m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

inline void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData)
{
	ASSERT(VertexData != nullptr && Math::IsAligned(VertexData, 16));

	size_t BufferSize = Math::AlignUp(NumVertices * VertexStride, 16);
	DynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

	//SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);
	memcpy(vb.DataPtr, VertexData, BufferSize);
	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.GpuAddress;
	VBView.SizeInBytes = (UINT)BufferSize;
	VBView.StrideInBytes = (UINT)VertexStride;

	m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

inline void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t* IndexData)
{
	ASSERT(IndexData != nullptr && Math::IsAligned(IndexData, 16));

	size_t BufferSize = Math::AlignUp(IndexCount * sizeof(uint16_t), 16);
	DynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

	//SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);
	memcpy(ib.DataPtr, IndexData, BufferSize);
	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.GpuAddress;
	IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	m_CommandList->IASetIndexBuffer(&IBView);
}

inline void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	//memcpy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16));
	m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

inline void GraphicsContext::SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV )
{
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

inline void GraphicsContext::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
	// request a upload buffer (constant buffer need to align 256B)
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize); 
	// copy data to constant buffer
    SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	// memcpy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

inline void GraphicsContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void* pConstants)
{
	m_CommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
}

inline void GraphicsContext::SetConstant(UINT RootEntry, UINT Offset, DWParam Val)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootEntry, Val.Uint, Offset);
}

inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
}

inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
}

inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
}

inline void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
}

inline void GraphicsContext::SetStencilRef(UINT ref)
{
	m_CommandList->OMSetStencilRef(ref);
}

inline void GraphicsContext::SetBlendFactor(Color BlendFactor)
{
	m_CommandList->OMSetBlendFactor(BlendFactor.GetPtr());
}

inline void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
{
	m_CommandList->IASetPrimitiveTopology(Topology);
}

