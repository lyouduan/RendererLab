#include "DepthBuffer.h"
#include "GraphicsCore.h"

void DepthBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	Create(Name, Width, Height, 1, Format, VidMemPtr);
}

void DepthBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	// create Desc for depthStencil
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ResourceDesc.SampleDesc.Count = NumSamples;

	// clear Value
	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	// add depth and stencil default value
	ClearValue.DepthStencil.Depth = m_ClearDepth;
	ClearValue.DepthStencil.Stencil = m_ClearStencil;
	CreateTextureResource(Graphics::g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Graphics::g_Device, Format);
}

void DepthBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format)
{
	ID3D12Resource* Resource = m_pResource.Get();

	// create depth view
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(Format);

	if (Resource->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		// allocate dsv descriptor
		m_hDSV[0] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_hDSV[1] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	// depth buffer
	{
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);
	}
	
	DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hDSV[2] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_hDSV[3] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		// stencil buffer
		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
	}
	else
	{
		m_hDSV[2] = m_hDSV[0];
		m_hDSV[3] = m_hDSV[1];
	}

	// record depth to srv, such as shadow map
	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_hDepthSRV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat(Format);
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_hDepthSRV);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hStencilSRV = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SRVDesc.Format = stencilReadFormat;
		SRVDesc.Texture2D.PlaneSlice = 1;

		Device->CreateShaderResourceView(Resource, &SRVDesc, m_hStencilSRV);
	}
}
 