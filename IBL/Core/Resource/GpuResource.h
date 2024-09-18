#pragma once
#include "pch.h"

class GpuResource
{
    friend class CommandContext;
    friend class GraphicsContext;
    //friend class ComputeContext;

public:
    GpuResource() :
        m_UsageState(D3D12_RESOURCE_STATE_COMMON),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1),
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
    {
    }
    
    GpuResource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
        m_pResource(pResource),
        m_UsageState(CurrentState),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1),
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL)
    {
    }

    ~GpuResource() { Destroy(); }

    virtual void Destroy()
    {
        m_pResource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        ++m_VersionID;
    }

    // override the operator
    ID3D12Resource* operator->() { return m_pResource.Get(); }
    const ID3D12Resource* operator->() const { return m_pResource.Get(); }

    // getter
    ID3D12Resource* GetResource() { return m_pResource.Get(); }
    const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

    ID3D12Resource** GetAddressOf() { return m_pResource.GetAddressOf(); }

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

    uint32_t GetVersionID() const { return m_VersionID; }

protected:

    Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;

    // current state
    D3D12_RESOURCE_STATES m_UsageState;
    // transition state
    D3D12_RESOURCE_STATES m_TransitioningState;
    // tracks Resource
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

    // Used to identify when a resource changes so descriptors can be copied etc.
    uint32_t m_VersionID = 0;
};
