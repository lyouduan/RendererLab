#include "pch.h"
#include "GameApp.h"
#include "GameCore.h"
#include "CommandContext.h"
#include "GraphicsCore.h"
#include "Display.h"
#include "BufferManager.h"
#include "GraphicsCommon.h"
#include "GameInput.h"
#include <DirectXColors.h>
#include "GeometryGenerator.h"
#include "TextureManager.h"
#include "DescriptorHeap.h"
#include <fstream>
#include <d3dcompiler.h>

using namespace Graphics;
using namespace DirectX;

GameApp::GameApp(void)
{
	m_Scissor.left = 0;
	m_Scissor.right = g_DisplayWidth;
	m_Scissor.top = 0;
	m_Scissor.bottom = g_DisplayHeight;

	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = static_cast<float>(g_DisplayWidth);
	m_Viewport.Height = static_cast<float>(g_DisplayHeight);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	 
	m_aspectRatio = static_cast<float>(g_DisplayHeight) / static_cast<float>(g_DisplayWidth);

}

void GameApp::Startup(void)
{
	// prepare material
	BuildMaterials();

	// load Textures
	LoadTextures();

	// prepare shape and add material
	BuildShapeGeometry();
	BuildBoxGeometry();
	BuildQuadGeometry();

	// build render items
	BuildShapeRenderItems();
	BuildSkyboxRenderItems();

	// build cubemap camera
	BuildCubeFaceCamera(0.0, 2.0, 0.0);

	// set PSO and Root Signature
	SetPsoAndRootSig();
	
}

void GameApp::Cleanup(void)
{
	for (auto& iter : m_AllRenders)
	{
		iter->Geo->m_VertexBuffer.Destroy();
		iter->Geo->m_IndexBuffer.Destroy();
	}

	m_Materials.clear();
	m_Textures.clear();
	m_Geometry.clear();
}

void GameApp::Update(float deltaT)
{
	// update camera 
	UpdateCamera(deltaT);

	// update passCB
	UpdatePassCB(deltaT);

	totalTime += deltaT * 0.1;

	// switch the scene
	if (GameInput::IsFirstPressed(GameInput::kKey_f1))
		m_bRenderShapes = !m_bRenderShapes;

}

void GameApp::RenderScene(void)
{
	GraphicsContext& gfxContext = GraphicsContext::Begin(L"");

	{
		// draw cubemap
		DrawSceneToCubeMap(gfxContext);

		// irradiance cube map
		ConvoluteCubeMap(gfxContext);

		// prefilter cube map
		PrefilterCubeMap(gfxContext);

		// brdf LUT
		Brdf2DLUT(gfxContext);
	}
	// 
	// scene pass
	// 
	// reset viewport and scissor
	gfxContext.SetViewportAndScissor(m_Viewport, m_Scissor);

	gfxContext.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	// clear dsv
	gfxContext.ClearDepthAndStencil(g_SceneDepthBuffer);
	// clear rtv
	g_DisplayPlane[g_CurrentBuffer].SetClearColor(Color(0.2, 0.2, 0.2, 1.0f));

	gfxContext.ClearColor(g_DisplayPlane[g_CurrentBuffer]);

	// set render target
	gfxContext.SetRenderTarget(g_DisplayPlane[g_CurrentBuffer].GetRTV(), g_SceneDepthBuffer.GetDSV());

	gfxContext.SetRootSignature(m_RootSignature);

	XMStoreFloat4x4(&passConstant.ViewProj, XMMatrixTranspose(camera.GetViewProjMatrix()));
	XMStoreFloat3(&passConstant.eyePosW, camera.GetPosition());
	gfxContext.SetDynamicConstantBufferView(1, sizeof(passConstant), &passConstant);

	// structured buffer
	gfxContext.SetBufferSRV(2, matBuffer);

	// srv tables
	gfxContext.SetDynamicDescriptors(5, 0, m_srvs.size(), &m_srvs[0]);

	// draw call
	//if (!m_bRenderShapes)
	{
		gfxContext.SetDynamicDescriptor(3, 0, g_IrradianceMapBuffer.GetSRV());
		gfxContext.SetDynamicDescriptor(4, 0, g_PrefilterBuffer.GetSRV());
		gfxContext.SetDynamicDescriptor(5, 0, g_Brdf2DLutBuffer.GetSRV());
		gfxContext.SetPipelineState(m_PSOs["opaque"]);
		DrawRenderItems(gfxContext, m_ShapeRenders[(int)RenderLayer::Opaque]);
	}
	
	// draw sky box at last
	gfxContext.SetPipelineState(m_PSOs["sky"]);
	{
		gfxContext.SetDynamicDescriptor(3, 0, g_SceneCubeMapBuffer.GetSRV());
		DrawRenderItems(gfxContext, m_SkyboxRenders[(int)RenderLayer::Skybox]);
	}

	//gfxContext.CopyBuffer(g_DisplayPlane[g_CurrentBuffer], g_Brdf2DLutBuffer);

	gfxContext.TransitionResource(g_DisplayPlane[g_CurrentBuffer], D3D12_RESOURCE_STATE_PRESENT);

	gfxContext.Finish();
}

void GameApp::SetPsoAndRootSig()
{
	// initialize root signature
	m_RootSignature.Reset(6, 1);
	m_RootSignature[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature[1].InitAsConstantBuffer(1, D3D12_SHADER_VISIBILITY_ALL);
	m_RootSignature[2].InitAsBufferSRV(0, D3D12_SHADER_VISIBILITY_ALL, 1);
	m_RootSignature[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 1);
	m_RootSignature[4].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);
	m_RootSignature[5].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 8);
	// sampler
	m_RootSignature.InitStaticSampler(0, Graphics::SamplerLinearWrapDesc, D3D12_SHADER_VISIBILITY_PIXEL);

	m_RootSignature.Finalize(L"root sinature", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// shader input layout
	D3D12_INPUT_ELEMENT_DESC mInputLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	DXGI_FORMAT ColorFormat = g_DisplayPlane[g_CurrentBuffer].GetFormat();
	DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();

	// shader 
	ComPtr<ID3DBlob> vertexBlob;
	ComPtr<ID3DBlob> pixelBlob;
	D3DReadFileToBlob(L"shader/VertexShader.cso", &vertexBlob);
	D3DReadFileToBlob(L"shader/PixelShader.cso", &pixelBlob);

	// PSO
	GraphicsPSO opaquePSO;
	opaquePSO.SetRootSignature(m_RootSignature);
	opaquePSO.SetRasterizerState(RasterizerDefaultCCw); // yz
	opaquePSO.SetBlendState(BlendDisable);
	opaquePSO.SetDepthStencilState(DepthStateReadWriteRZ);// reversed-z
	opaquePSO.SetInputLayout(_countof(mInputLayout), mInputLayout);
	opaquePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	opaquePSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
	opaquePSO.SetVertexShader(vertexBlob);
	opaquePSO.SetPixelShader(pixelBlob);
	opaquePSO.Finalize();
	m_PSOs["opaque"] = opaquePSO;

	GraphicsPSO transpacrentPSO = opaquePSO;
	auto blend = Graphics::BlendTraditional;
	blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	transpacrentPSO.SetBlendState(blend);
	transpacrentPSO.Finalize();
	m_PSOs["transparent"] = transpacrentPSO;

	GraphicsPSO alphaTestedPSO = opaquePSO;
	auto rater = RasterizerDefault;
	rater.CullMode = D3D12_CULL_MODE_NONE; // not cull 
	alphaTestedPSO.SetRasterizerState(rater);
	alphaTestedPSO.Finalize();
	m_PSOs["alphaTested"] = alphaTestedPSO;

	// cubemap 
	ColorFormat = g_SceneCubeMapBuffer.GetFormat();
	DepthFormat = g_CubeMapDepthBuffer.GetFormat();

	// shader 
	ComPtr<ID3DBlob> skyboxVS;
	ComPtr<ID3DBlob> skyboxPS;
	D3DReadFileToBlob(L"shader/skyboxVS.cso", &skyboxVS);
	D3DReadFileToBlob(L"shader/skyboxPS.cso", &skyboxPS);

	// pso
	GraphicsPSO skyPSO = opaquePSO;
	auto depthDesc = DepthStateReadWriteRZ;
	depthDesc.DepthFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL; // 大于等于

	rater.CullMode = D3D12_CULL_MODE_NONE; // 禁止
	skyPSO.SetDepthStencilState(depthDesc);
	skyPSO.SetRasterizerState(rater);
	skyPSO.SetRenderTargetFormat(ColorFormat, DepthFormat);
	skyPSO.SetVertexShader(skyboxVS);
	skyPSO.SetPixelShader(skyboxPS);
	skyPSO.Finalize();
	m_PSOs["sky"] = skyPSO;

	// shader 
	ComPtr<ID3DBlob> cubemapVS;
	ComPtr<ID3DBlob> cubemapPS;
	D3DReadFileToBlob(L"shader/cubemapVS.cso", &cubemapVS);
	D3DReadFileToBlob(L"shader/cubemapPS.cso", &cubemapPS);
	// pso
	GraphicsPSO cubemapPSO = skyPSO;
	cubemapPSO.SetRasterizerState(rater);
	cubemapPSO.SetRenderTargetFormat(g_SceneCubeMapBuffer.GetFormat(), DepthFormat);
	cubemapPSO.SetVertexShader(cubemapVS);
	cubemapPSO.SetPixelShader(cubemapPS);
	cubemapPSO.Finalize();
	m_PSOs["cubemap"] = cubemapPSO;

	// shader 
	ComPtr<ID3DBlob> irradianceMapPS;
	D3DReadFileToBlob(L"shader/irradianceMapPS.cso", &irradianceMapPS);
	// pso
	GraphicsPSO irradiancePSO = opaquePSO;
	irradiancePSO.SetRasterizerState(rater);
	irradiancePSO.SetRenderTargetFormat(g_IrradianceMapBuffer.GetFormat(), DepthFormat);
	irradiancePSO.SetVertexShader(cubemapVS);
	irradiancePSO.SetPixelShader(irradianceMapPS);
	irradiancePSO.Finalize();
	m_PSOs["irradiance"] = irradiancePSO;

	// shader 
	ComPtr<ID3DBlob> prefilterPS;
	D3DReadFileToBlob(L"shader/prefilterPS.cso", &prefilterPS);
	// pso
	GraphicsPSO prefilterPSO = opaquePSO;
	prefilterPSO.SetRasterizerState(rater);
	prefilterPSO.SetRenderTargetFormat(g_PrefilterBuffer.GetFormat(), DepthFormat);
	prefilterPSO.SetVertexShader(cubemapVS);
	prefilterPSO.SetPixelShader(prefilterPS);
	prefilterPSO.Finalize();
	m_PSOs["prefilter"] = prefilterPSO;


	// shader 
	ComPtr<ID3DBlob> brdfVS;
	ComPtr<ID3DBlob> brdfPS;
	D3DReadFileToBlob(L"shader/brdfVS.cso", &brdfVS);
	D3DReadFileToBlob(L"shader/brdfPS.cso", &brdfPS);
	// pso
	GraphicsPSO brdfPSO = opaquePSO;
	//brdfPSO.SetRasterizerState(rater);
	brdfPSO.SetRenderTargetFormat(g_Brdf2DLutBuffer.GetFormat(), DepthFormat);
	brdfPSO.SetVertexShader(brdfVS);
	brdfPSO.SetPixelShader(brdfPS);
	brdfPSO.Finalize();
	m_PSOs["brdf"] = brdfPSO;
}

void GameApp::DrawRenderItems(GraphicsContext& gfxContext, std::vector<RenderItem*>& items)
{
	ObjConstants objConstants;
	for (auto& iter : items)
	{
		gfxContext.SetPrimitiveTopology(iter->PrimitiveType);
		gfxContext.SetVertexBuffer(0, iter->Geo->m_VertexBuffer.VertexBufferView());
		gfxContext.SetIndexBuffer(iter->Geo->m_IndexBuffer.IndexBufferView());

		XMStoreFloat4x4(&objConstants.World, XMMatrixTranspose(iter->World)); // hlsl 列主序矩阵
		XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(iter->TexTransform)); // hlsl 列主序矩阵
		XMStoreFloat4x4(&objConstants.MatTransform, XMMatrixTranspose(iter->MatTransform)); // hlsl 列主序矩阵
		objConstants.MaterialIndex = iter->ObjCBIndex;

		gfxContext.SetDynamicConstantBufferView(0, sizeof(objConstants), &objConstants);

		gfxContext.DrawIndexedInstanced(iter->IndexCount, 1, iter->StartIndexLocation, iter->BaseVertexLocation, 0);
	}
}

void GameApp::DrawSceneToCubeMap(GraphicsContext& gfxContext)
{
	auto width = Graphics::g_SceneCubeMapBuffer.GetWidth();
	auto height = Graphics::g_SceneCubeMapBuffer.GetHeight();
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (LONG)width, (LONG)height };
	gfxContext.SetViewportAndScissor(mViewport, mScissorRect);

	gfxContext.TransitionResource(g_SceneCubeMapBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.TransitionResource(g_CubeMapDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	//clear rtv
	g_SceneCubeMapBuffer.SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
	gfxContext.ClearColor(g_SceneCubeMapBuffer);

	gfxContext.SetRootSignature(m_RootSignature);

	// structured buffer
	gfxContext.SetBufferSRV(2, matBuffer);

	// srv tables
	gfxContext.SetDynamicDescriptor(3, 0, m_cubeMap[0].GetSRV());
	gfxContext.SetDynamicDescriptors(5, 0, m_srvs.size(), &m_srvs[0]);

	for (int i = 0; i < 6; ++i)
	{
		// clear dsv
		gfxContext.ClearDepthAndStencil(g_CubeMapDepthBuffer);
		// set render target
		gfxContext.SetRenderTarget(g_SceneCubeMapBuffer.GetRTV(i), g_CubeMapDepthBuffer.GetDSV());

		// update passCB
		XMStoreFloat4x4(&passConstant.ViewProj, XMMatrixTranspose(cubeCamera[i].GetViewProjMatrix()));
		XMStoreFloat3(&passConstant.eyePosW, cubeCamera[i].GetPosition());
		gfxContext.SetDynamicConstantBufferView(1, sizeof(passConstant), &passConstant);

		// draw call
		gfxContext.SetPipelineState(m_PSOs["cubemap"]);
		DrawRenderItems(gfxContext, m_SkyboxRenders[(int)RenderLayer::Cubemap]);
	}

	gfxContext.TransitionResource(g_SceneCubeMapBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, true);
}

void GameApp::ConvoluteCubeMap(GraphicsContext& gfxContext)
{
	auto width = Graphics::g_IrradianceMapBuffer.GetWidth();
	auto height = Graphics::g_IrradianceMapBuffer.GetHeight();
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (LONG)width, (LONG)height };
	gfxContext.SetViewportAndScissor(mViewport, mScissorRect);

	gfxContext.TransitionResource(g_IrradianceMapBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.TransitionResource(g_CubeMapDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	//clear rtv
	g_IrradianceMapBuffer.SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
	gfxContext.ClearColor(g_IrradianceMapBuffer);

	gfxContext.SetRootSignature(m_RootSignature);

	// structured buffer
	gfxContext.SetBufferSRV(2, matBuffer);

	// srv tables
	gfxContext.SetDynamicDescriptor(3, 0, g_SceneCubeMapBuffer.GetSRV());
	gfxContext.SetDynamicDescriptors(5, 0, m_srvs.size(), &m_srvs[0]);

	for (int i = 0; i < 6; ++i)
	{
		// clear dsv
		gfxContext.ClearDepthAndStencil(g_CubeMapDepthBuffer);
		// set render target
		gfxContext.SetRenderTarget(g_IrradianceMapBuffer.GetRTV(i), g_CubeMapDepthBuffer.GetDSV());

		// update passCB
		XMStoreFloat4x4(&passConstant.ViewProj, XMMatrixTranspose(cubeCamera[i].GetViewProjMatrix()));
		XMStoreFloat3(&passConstant.eyePosW, cubeCamera[i].GetPosition());
		gfxContext.SetDynamicConstantBufferView(1, sizeof(passConstant), &passConstant);

		// draw call
		gfxContext.SetPipelineState(m_PSOs["irradiance"]);
		DrawRenderItems(gfxContext, m_SkyboxRenders[(int)RenderLayer::Cubemap]);
	}

	gfxContext.TransitionResource(g_IrradianceMapBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, true);
}

void GameApp::PrefilterCubeMap(GraphicsContext& gfxContext)
{
	auto width = Graphics::g_PrefilterBuffer.GetWidth();
	auto height = Graphics::g_PrefilterBuffer.GetHeight();
	

	gfxContext.TransitionResource(g_PrefilterBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.TransitionResource(g_CubeMapDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	//clear rtv
	g_PrefilterBuffer.SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
	gfxContext.ClearColor(g_PrefilterBuffer);

	gfxContext.SetRootSignature(m_RootSignature);

	// structured buffer
	gfxContext.SetBufferSRV(2, matBuffer);

	// srv tables
	gfxContext.SetDynamicDescriptor(3, 0, g_SceneCubeMapBuffer.GetSRV());
	gfxContext.SetDynamicDescriptors(5, 0, m_srvs.size(), &m_srvs[0]);

	passConstant.roughness = 0.1f;
	auto mips = g_PrefilterBuffer.GetResource()->GetDesc().MipLevels;
	for (int mip = 0; mip < mips; ++mip)
	{
		auto mipwidth = (float)(width / pow(2, mip));
		auto mipheight = (float)(height / pow(2, mip));
		D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)mipwidth, (float)mipheight, 0.0f, 1.0f};
		D3D12_RECT mScissorRect = { 0, 0, (LONG)mipwidth, (LONG)mipheight };

		gfxContext.SetViewportAndScissor(mViewport, mScissorRect);

		passConstant.resulotion = mipwidth;
		passConstant.roughness = 0.1 + 0.2 * mip;
		for (int i = 0; i < 6; ++i)
		{
			// clear dsv
			gfxContext.ClearDepthAndStencil(g_CubeMapDepthBuffer);
			// set render target
			gfxContext.SetRenderTarget(g_PrefilterBuffer.GetRTV(mip * 6 + i), g_CubeMapDepthBuffer.GetDSV());

			// update passCB
			XMStoreFloat4x4(&passConstant.ViewProj, XMMatrixTranspose(cubeCamera[i].GetViewProjMatrix()));
			XMStoreFloat3(&passConstant.eyePosW, cubeCamera[i].GetPosition());
			gfxContext.SetDynamicConstantBufferView(1, sizeof(passConstant), &passConstant);

			// draw call
			gfxContext.SetPipelineState(m_PSOs["prefilter"]);
			DrawRenderItems(gfxContext, m_SkyboxRenders[(int)RenderLayer::Cubemap]);
		}
	}
	gfxContext.TransitionResource(g_PrefilterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, true);
}

void GameApp::Brdf2DLUT(GraphicsContext& gfxContext)
{
	auto width = Graphics::g_Brdf2DLutBuffer.GetWidth();
	auto height = Graphics::g_Brdf2DLutBuffer.GetHeight();
	D3D12_VIEWPORT mViewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	D3D12_RECT mScissorRect = { 0, 0, (LONG)width, (LONG)height };
	gfxContext.SetViewportAndScissor(mViewport, mScissorRect);

	gfxContext.TransitionResource(g_Brdf2DLutBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
	gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE, true);

	//clear rtv
	g_Brdf2DLutBuffer.SetClearColor(Color(1.0f, 1.0f, 1.0f, 0.0f));
	gfxContext.ClearColor(g_Brdf2DLutBuffer);

	gfxContext.SetRootSignature(m_RootSignature);

	// structured buffer
	gfxContext.SetBufferSRV(2, matBuffer);

	// srv tables
	gfxContext.SetDynamicDescriptor(3, 0, g_SceneCubeMapBuffer.GetSRV());
	gfxContext.SetDynamicDescriptors(4, 0, m_srvs.size(), &m_srvs[0]);

	{
		// clear dsv
		gfxContext.ClearDepthAndStencil(g_SceneDepthBuffer);
		// set render target
		gfxContext.SetRenderTarget(g_Brdf2DLutBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());

		// draw call
		gfxContext.SetPipelineState(m_PSOs["brdf"]);
		DrawRenderItems(gfxContext, m_SkyboxRenders[(int)RenderLayer::Quad]);
	}

	gfxContext.TransitionResource(g_Brdf2DLutBuffer, D3D12_RESOURCE_STATE_GENERIC_READ, true);
}

void GameApp::BuildCubeFaceCamera(float x, float y, float z)
{
	// Generate the cube map about the given position.
	Math::Vector3 center{ 0, 0, 0 };

	// look along each coordinate axis;
	Math::Vector3 targets[6] =
	{
		{ -1.0f,  +0.0f,  +0.0f}, // +X
		{ +1.0f,  +0.0f,  +0.0f}, // -X
		{ +0.0f,  -1.0f,  +0.0f}, // +Y
		{ +0.0f,  +1.0f,  +0.0f}, // -Y
		{ +0.0f,  +0.0f,  -1.0f}, // -Z
		{ +0.0f,  +0.0f,  +1.0f}, // +Z

	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	Math::Vector3 ups[6] =
	{
		{ 0.0f, -1.0f,  0.0f },    // +X
		{ 0.0f, -1.0f,  0.0f },    // -X
		{ 0.0f,  0.0f,  1.0f },    // -Y
		{ 0.0f,  0.0f, -1.0f },    // +Y
		{ 0.0f, -1.0f,  0.0f },    // -Z
		{ 0.0f, -1.0f,  0.0f },    // +Z
	};

	for (int i = 0; i < 6; ++i)
	{
		cubeCamera[i].SetEyeAtUp(
			center,
			targets[i],
			ups[i]
		);
		cubeCamera[i].SetPerspectiveMatrix(XM_PI*0.5f, 1, 0.1, 1000.0f); // 45
		cubeCamera[i].Update();
	}
}

void GameApp::BuildShapeRenderItems()
{
	XMMATRIX brickTexTransform = XMMatrixScaling(1.0f, 1.0f, 1.0f);
	for (int row = 0; row < 7; ++row)
	{
		for (int col = 0; col < 7; ++col)
		{
			XMMATRIX SphereWorld = XMMatrixTranslation((float)(col - (7 / 2)) * 2.5,
				(float)(row - (7 / 2)) * 2.5,
				-2.0f);
			auto SphereRitem = std::make_unique<RenderItem>();

			SphereRitem->World = XMMatrixIdentity() * XMMatrixScaling(2.0f, 2.0f, 2.0f) * SphereWorld;
			SphereRitem->TexTransform = brickTexTransform;
			SphereRitem->ObjCBIndex = row * 7 + col;
			//SphereRitem->Mat = m_Materials[SphereRitem->ObjCBIndex].get();
			SphereRitem->Geo = m_Geometry["shapeGeo"].get();
			SphereRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			SphereRitem->IndexCount = SphereRitem->Geo->DrawArgs["sphere"].IndexCount;
			SphereRitem->StartIndexLocation = SphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
			SphereRitem->BaseVertexLocation = SphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

			m_ShapeRenders[(int)RenderLayer::Opaque].push_back(SphereRitem.get());

			m_AllRenders.push_back(std::move(SphereRitem));
		}
	}

}

void GameApp::BuildSkyboxRenderItems()
{
	auto box = std::make_unique<RenderItem>();
	box->World = XMMatrixIdentity();
	box->ObjCBIndex = m_Materials.size()-1;
	//box->Mat = m_Materials["sky"].get();
	box->Geo = m_Geometry["shapeGeo"].get();
	box->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	box->IndexCount = box->Geo->DrawArgs["box"].IndexCount;
	box->BaseVertexLocation = box->Geo->DrawArgs["box"].BaseVertexLocation;
	box->StartIndexLocation = box->Geo->DrawArgs["box"].StartIndexLocation;

	m_SkyboxRenders[(int)RenderLayer::Cubemap].push_back(box.get());
	m_SkyboxRenders[(int)RenderLayer::Skybox].push_back(box.get());

	auto quad = std::make_unique<RenderItem>();
	quad->World = XMMatrixIdentity();
	quad->ObjCBIndex = m_Materials.size() - 1;
	//box->Mat = m_Materials["sky"].get();
	quad->Geo = m_Geometry["quadGeo"].get();
	quad->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quad->IndexCount = quad->Geo->DrawArgs["quad"].IndexCount;
	quad->BaseVertexLocation = quad->Geo->DrawArgs["quad"].BaseVertexLocation;
	quad->StartIndexLocation = quad->Geo->DrawArgs["quad"].StartIndexLocation;

	m_SkyboxRenders[(int)RenderLayer::Quad].push_back(quad.get());

	m_AllRenders.push_back(std::move(box));
	m_AllRenders.push_back(std::move(quad));
}

void GameApp::BuildShapeGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// Cache the starting index for each object in the concatenated index buffer.
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	auto totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].position = box.Vertices[i].Position;
		vertices[k].normal = box.Vertices[i].Normal;
		vertices[k].tex = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].position = grid.Vertices[i].Position;
		vertices[k].normal = grid.Vertices[i].Normal;
		vertices[k].tex = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].position = sphere.Vertices[i].Position;
		vertices[k].normal = sphere.Vertices[i].Normal;
		vertices[k].tex = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].position = cylinder.Vertices[i].Position;
		vertices[k].normal = cylinder.Vertices[i].Normal;
		vertices[k].tex = cylinder.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "shapeGeo";
	geo->m_VertexBuffer.Create(L"vertex buff", (UINT)vertices.size(), sizeof(Vertex), vertices.data());
	geo->m_IndexBuffer.Create(L"Index Buffer", (UINT)indices.size(), sizeof(std::uint16_t), indices.data());

	geo->DrawArgs["box"] = std::move(boxSubmesh);
	geo->DrawArgs["grid"] = std::move(gridSubmesh);
	geo->DrawArgs["sphere"] = std::move(sphereSubmesh);
	geo->DrawArgs["cylinder"] = std::move(cylinderSubmesh);

	m_Geometry["shapeGeo"] = std::move(geo);
}

void GameApp::BuildBoxGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(6.0f, 6.0f, 6.0f, 3);

	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].position = p;
		vertices[i].normal = box.Vertices[i].Normal;
		vertices[i].tex = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "boxGeo";
	geo->m_VertexBuffer.Create(L"vertex buff", (UINT)vertices.size(), sizeof(Vertex), vertices.data());
	geo->m_IndexBuffer.Create(L"Index Buffer", (UINT)indices.size(), sizeof(std::uint16_t), indices.data());

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["sbox"] = std::move(submesh);

	m_Geometry["boxGeo"] = std::move(geo);
}

void GameApp::BuildQuadGeometry()
{
	//GeometryGenerator geoGen;
	//GeometryGenerator::MeshData quad = geoGen.CreateQuad(-1.0, 1.0, 2, 2, 1.0);
	//std::vector<Vertex> vertices.resize(quad.Vertices.size());
	//for (int i = 0; i < quad.Vertices.size(); ++i)
	//{
	//	vertices[i].position = quad.Vertices[i].Position;
	//	vertices[i].tex = quad.Vertices[i].TexC;
	//}
	//std::vector<std::uint16_t> indices = quad.GetIndices16();

	std::vector<Vertex> vertices;
	vertices.push_back({ XMFLOAT3{ -1.0f, 1.0f, 1.0f }, XMFLOAT3{ 0.0f,  1.0f, 0.0f }, XMFLOAT2{ 0.0f, 1.0f } });
	vertices.push_back({ XMFLOAT3{ -1.0f,-1.0f, 1.0f }, XMFLOAT3{ 0.0f,  1.0f, 0.0f }, XMFLOAT2{ 0.0f, 0.0f } });
	vertices.push_back({ XMFLOAT3{  1.0f, 1.0f, 1.0f }, XMFLOAT3{ 0.0f,  1.0f, 0.0f }, XMFLOAT2{ 1.0f, 1.0f } });
	vertices.push_back({ XMFLOAT3{  1.0f,-1.0f, 1.0f }, XMFLOAT3{ 0.0f,  1.0f, 0.0f }, XMFLOAT2{ 1.0f, 0.0f } });
		
	std::vector<std::uint16_t> indices = { 0, 1, 2, 1, 3, 2}; // 右手 逆时针
	
	auto geo = std::make_unique<MeshGeometry>();
	geo->name = "quadGeo";
	geo->m_VertexBuffer.Create(L"vertex buff", (UINT)vertices.size(), sizeof(Vertex), vertices.data());
	geo->m_IndexBuffer.Create(L"Index Buffer", (UINT)indices.size(), sizeof(std::uint16_t), indices.data());

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["quad"] = std::move(submesh);

	m_Geometry["quadGeo"] = std::move(geo);
}

void GameApp::BuildMaterials()
{
	for (int row = 0; row < 7; ++row)
	{
		for (int col = 0; col < 7; ++col)
		{
			auto red = std::make_unique<Material>();
			red->Name = m_Materials.size();
			red->DiffuseMapIndex = m_Materials.size();
			red->DiffuseAlbedo = XMFLOAT4(0.8f, 0.0f, 0.0f, 1.0f);
			red->metallic = std::clamp((float)(col / 7.0f), 0.05f, 1.0f);
			red->FresnelR0 = XMFLOAT3(0.1, 0.1, 0.1);
			red->Roughness = std::clamp((float)(row / 7.0f), 0.05f, 1.0f);

			m_Materials[red->Name] = std::move(red);
		}
	}

	auto sky = std::make_unique<Material>();
	sky->Name = "sky";
	sky->DiffuseMapIndex = m_Materials.size();
	sky->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	sky->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sky->Roughness = 0.5f;
	m_Materials[sky->Name] = std::move(sky);

	std::vector<MaterialConstants> mat;
	MaterialConstants temp;
	for (auto& m : m_Materials)
	{
		temp.DiffuseAlbedo = m.second->DiffuseAlbedo;
		temp.DiffuseMapIndex = m.second->DiffuseMapIndex;
		XMStoreFloat4x4(&temp.MatTransform, XMMatrixTranspose(m.second->MatTransform));

		temp.metallic = m.second->metallic;
		temp.Roughness = m.second->Roughness;
		mat.push_back(temp);
	}

	std::sort(mat.begin(), mat.end(), 
		[](const MaterialConstants& a, const MaterialConstants& b) 
		{return a.DiffuseMapIndex < b.DiffuseMapIndex; }
	);
	matBuffer.Create(L"material buffer", (UINT)m_Materials.size(), sizeof(MaterialConstants), mat.data());
}

void GameApp::LoadTextures()
{
	// 需要把TextureRef对象保存下来，出了有效域对象销毁，handle无效了
	TextureManager::Initialize(L"../textures/");

	TextureRef white1x1Tex = TextureManager::LoadDDSFromFile(L"white1x1.dds");
	if (white1x1Tex.IsValid())
		m_Textures.push_back(white1x1Tex);

	TextureRef cubeMap = TextureManager::LoadDDSFromFile(L"newport_loft.dds");
	if (cubeMap.IsValid())
		m_cubeMap.push_back(cubeMap);

	Utility::Printf("Found %u textures\n", m_Textures.size());

	//m_srvs.resize(m_Textures.size());
	for (auto& t : m_Textures)
		m_srvs.push_back(t.GetSRV());

}

void GameApp::UpdatePassCB(float deltaT)
{
	// up
	XMStoreFloat4x4(&passConstant.ViewProj, XMMatrixTranspose(camera.GetViewProjMatrix())); // hlsl 列主序矩阵
	XMStoreFloat3(&passConstant.eyePosW, camera.GetPosition());
	passConstant.ambientLight = { 0.2f, 0.2f, 0.2f, 1.0f };

	passConstant.Lights[0].Direction = { 0.57735f, 0.57735f, 0.57735f };
	passConstant.Lights[0].Strength = { 1.0f, 1.0f, 1.0f };
	passConstant.Lights[0].Position = { -10.0f, 10.0f, 10.0f };

	passConstant.Lights[1].Direction = { 0.57735f, -0.57735f, 0.57735f };
	passConstant.Lights[1].Strength = { 1.0f, 1.0f, 1.0f };
	passConstant.Lights[1].Position = { 10.0f, 10.0f, 10.0f };

	passConstant.Lights[2].Direction = { -0.57735f, -0.57735f, 0.57735f };
	passConstant.Lights[2].Strength = { 1.0f, 1.0f, 1.0f };
	passConstant.Lights[2].Position = { -10.0f, -10.0f, 10.0f };

	passConstant.Lights[3].Direction = { -0.57735f, 0.57735f, 0.57735f };
	passConstant.Lights[3].Strength = { 1.0f, 1.0f, 1.0f };
	passConstant.Lights[3].Position = { 10.0f, -10.0f, 10.0f };
}

void GameApp::UpdateCamera(float deltaT)
{
	if (GameInput::IsPressed(GameInput::kMouse0) || GameInput::IsPressed(GameInput::kMouse1)) {
		// Make each pixel correspond to a quarter of a degree.
		float dx = m_xLast - GameInput::GetAnalogInput(GameInput::kAnalogMouseX);
		float dy = m_yLast - GameInput::GetAnalogInput(GameInput::kAnalogMouseY);

		if (GameInput::IsPressed(GameInput::kMouse0))
		{
			// Update angles based on input to orbit camera around box.
			m_xRotate += (dx - m_xDiff);
			m_yRotate += (dy - m_yDiff);
			m_yRotate = (std::max)(-XM_PIDIV2 + 0.1f, m_yRotate);
			m_yRotate = (std::min)(XM_PIDIV2 - 0.1f, m_yRotate);
		}
		else
		{
			m_radius += dx - dy - (m_xDiff - m_yDiff);
		}

		m_xDiff = dx;
		m_yDiff = dy;

		m_xLast += GameInput::GetAnalogInput(GameInput::kAnalogMouseX);
		m_yLast += GameInput::GetAnalogInput(GameInput::kAnalogMouseY);
	}
	else
	{
		m_xDiff = 0.0f;
		m_yDiff = 0.0f;
		m_xLast = 0.0f;
		m_yLast = 0.0f;
	}

	float x = m_radius * cosf(m_yRotate) * sinf(m_xRotate);
	float y = m_radius * sinf(m_yRotate);
	float z = -m_radius * cosf(m_yRotate) * cosf(m_xRotate);

	camera.SetEyeAtUp({x,y,z}, Math::Vector3(Math::kZero), Math::Vector3(Math::kYUnitVector));
	camera.SetAspectRatio(m_aspectRatio);
	camera.Update();
}
