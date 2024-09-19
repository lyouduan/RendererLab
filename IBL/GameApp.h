#pragma once

#include "GameCore.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "GpuBuffer.h"
#include <DirectXMath.h>
#include "Waves.h"
#include "d3dUtil.h"
#include <memory>
#include "TextureManager.h"
#include "Camera.h"

enum class RenderLayer : int
{
	Opaque = 0,
	AlphaTested,
	Transparent,
	OpaqueDynamicReflectors,
	Cubemap,
	Skybox,
	Quad,
	Count
};

// render item
struct RenderItem
{
	RenderItem() = default;
	DirectX::XMMATRIX World = XMMatrixIdentity();
	DirectX::XMMATRIX TexTransform = XMMatrixIdentity();
	DirectX::XMMATRIX MatTransform = XMMatrixIdentity();

	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	MeshGeometry* Geo = nullptr;

	Material* Mat = nullptr;

	// Index into GPU constant buffer corresponding to the ObjectCB for this render item.
	UINT ObjCBIndex = -1; 

	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	UINT BaseVertexLocation = 0;

};

class GraphicsContext;

class GameApp : public GameCore::IGameApp
{
public:

	GameApp(void);

	virtual void Startup(void) override;
	virtual void Cleanup(void) override;

	virtual void Update(float deltaT) override;
	virtual void RenderScene(void) override;

private:

	void SetPsoAndRootSig();

	void DrawRenderItems(GraphicsContext& gfxContext, std::vector<RenderItem*>& items);
	void DrawSceneToCubeMap(GraphicsContext& gfxContext);
	void ConvoluteCubeMap(GraphicsContext& gfxContext);
	void PrefilterCubeMap(GraphicsContext& gfxContext);
	void Brdf2DLUT(GraphicsContext& gfxContext);

	void BuildCubeFaceCamera(float x=0.0, float y=0.0, float z=0.0);

	void BuildShapeRenderItems();
	void BuildSkyboxRenderItems();

	void BuildShapeGeometry();
	void BuildBoxGeometry();
	void BuildQuadGeometry();
	
	void BuildMaterials();
	void LoadTextures();

	void UpdatePassCB(float deltaT);
	void UpdateCamera(float deltaT);

	RootSignature m_RootSignature;

	std::unordered_map<std::string, GraphicsPSO> m_PSOs;

	// switch render scene
	bool m_bRenderShapes = true;

	// List of all the render items.
	std::vector <RenderItem*> m_ShapeRenders[(int)RenderLayer::Count];
	std::vector <RenderItem*> m_SkyboxRenders[(int)RenderLayer::Count];

	std::vector < std::unique_ptr<RenderItem>> m_AllRenders;

	// materials
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;

	// geometry
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometry;
	// texture manager
	std::vector<TextureRef> m_Textures; // work
	std::vector<TextureRef> m_cubeMap; // work
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_srvs;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_Scissor;
	float m_aspectRatio;

	DirectX::XMMATRIX m_View;
	DirectX::XMMATRIX m_Projection;

	PassConstants passConstant;
	StructuredBuffer matBuffer;

	float totalTime = 0;

	// camera
	Math::Camera camera;
	// cubeMap camera
	Math::Camera cubeCamera[6];

	float m_radius = 5.0f;
	// x方向弧度
	float m_xRotate = 0.0f;
	float m_xLast = 0.0f;
	float m_xDiff = 0.0f;
	// y方向弧度
	float m_yRotate = 0.0f;
	float m_yLast = 0.0f;
	float m_yDiff = 0.0f;

	float mSunTheta = 0.25f * DirectX::XM_PI;
	float mSunPhi = DirectX::XM_PIDIV4;
};