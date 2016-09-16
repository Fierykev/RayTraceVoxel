#include "Shadow.h"
#include "Shader.h"

#include "Terrain.h"

#define SHADOWWIDTH 500
#define SHADOWHEIGHT 500

ID3D11VertexShader* shadowvertexshader = NULL;
ID3D11InputLayout* shadowvertexlayout = NULL;
ID3D11Buffer* globalshadowbuffer = NULL;
ID3D11Buffer* globalshadowgenbuffer = NULL;
ID3D11Buffer* globaltexturegenbuffer = NULL;

ID3D11RasterizerState* regsceneRS;
ID3D11RasterizerState* shadowRS;

// TMP

float bias = .0003f;

struct StencilCB
{
	D3DXMATRIX worldviewproj;
};

struct ShadowCB
{
	D3DXMATRIX lightviewproj;
	D3DXVECTOR3 lightpos;
	FLOAT shadowbias;
};

struct ShadowTexCB
{
	INT width;
	INT height;
};

ShadowMap::ShadowMap()
{

}

ShadowMap::~ShadowMap()
{

}

HRESULT LoadShadowShader(ID3D11Device* pd3dDevice)
{
	HRESULT hr;

	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	ID3DBlob* shadowmapbuffer = NULL;

	V_RETURN(CompileShaderFromFile(L"Shader/ShadowMap.hlsl", "mainVS", "vs_4_1", &shadowmapbuffer));

	// Create the shaders

	V_RETURN(pd3dDevice->CreateVertexShader(shadowmapbuffer->GetBufferPointer(),
		shadowmapbuffer->GetBufferSize(), NULL, &shadowvertexshader));

	// Create layout

	V_RETURN(pd3dDevice->CreateInputLayout(
		layout, ARRAYSIZE(layout),
		shadowmapbuffer->GetBufferPointer(),
		shadowmapbuffer->GetBufferSize(),
		&shadowvertexlayout));
	DXUT_SetDebugName(shadowvertexlayout, "CascadedShadowsManager");

	SAFE_RELEASE(shadowmapbuffer);

	// Create Buffer

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	
	Desc.ByteWidth = ComputeCBufferSize(sizeof(StencilCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &globalshadowbuffer));
	DXUT_SetDebugName(globalshadowbuffer, "GlobalShadowBuffer");

	// Create References for the General Shadow Header
	Desc.ByteWidth = ComputeCBufferSize(sizeof(ShadowCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &globalshadowgenbuffer));
	DXUT_SetDebugName(globalshadowgenbuffer, "GlobalShadowGeneralBuffer");

	Desc.ByteWidth = ComputeCBufferSize(sizeof(ShadowTexCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &globaltexturegenbuffer));
	DXUT_SetDebugName(globaltexturegenbuffer, "GlobalShadowTextureGeneralBuffer");
	
	D3D11_RASTERIZER_DESC drd =
	{
		D3D11_FILL_SOLID,//D3D11_FILL_MODE FillMode;
		D3D11_CULL_NONE,//D3D11_CULL_MODE CullMode;
		FALSE,//BOOL FrontCounterClockwise;
		0,//INT DepthBias;
		0.0,//FLOAT DepthBiasClamp;
		0.0,//FLOAT SlopeScaledDepthBias;
		TRUE,//BOOL DepthClipEnable;
		FALSE,//BOOL ScissorEnable;
		TRUE,//BOOL MultisampleEnable;
		FALSE//BOOL AntialiasedLineEnable;   
	};

	pd3dDevice->CreateRasterizerState(&drd, &regsceneRS);
	DXUT_SetDebugName(regsceneRS, "CSM Scene");

	// Setting the slope scale depth biase greatly decreases surface acne and incorrect self shadowing.
	drd.SlopeScaledDepthBias = 1.0;
	pd3dDevice->CreateRasterizerState(&drd, &shadowRS);
	DXUT_SetDebugName(shadowRS, "CSM Shadow");

	return hr;
}

HRESULT ShadowMap::CreateShadowShader(ID3D11Device* pd3dDevice)
{
	HRESULT hr;

	width = SHADOWWIDTH;
	height = SHADOWHEIGHT;

	D3D11_TEXTURE2D_DESC dtd =
	{
		width, // Width;
		height, // Height;
		1, // MipLevels;
		1, // ArraySize;
		DXGI_FORMAT_R32_TYPELESS, // DXGI_FORMAT;
		1, // DXGI_SAMPLE_DESC;
		0,
		D3D11_USAGE_DEFAULT, // D3D11_USAGE;
		D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, //BindFlags;
		0, // CPUAccessFlags;
		0 // MiscFlags;    
	};

	V_RETURN(pd3dDevice->CreateTexture2D(&dtd, NULL, &shadowmaptex));
	DXUT_SetDebugName(shadowmaptex, "ShadowMapTexture");

	D3D11_DEPTH_STENCIL_VIEW_DESC  dsvd =
	{
		DXGI_FORMAT_D32_FLOAT,
		D3D11_DSV_DIMENSION_TEXTURE2D,
		0
	};

	V_RETURN(pd3dDevice->CreateDepthStencilView(shadowmaptex, &dsvd, &stencilviewSV));
	DXUT_SetDebugName(stencilviewSV, "ShadowMapStencil");

	D3D11_SHADER_RESOURCE_VIEW_DESC dsrvd =
	{
		DXGI_FORMAT_R32_FLOAT,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		0,
		0
	};
	dsrvd.Texture2D.MipLevels = 1;

	V_RETURN(pd3dDevice->CreateShaderResourceView(shadowmaptex, &dsrvd, &stencilviewSR));
	DXUT_SetDebugName(stencilviewSR, "ShadowMapStencil SR");

	lightview.Height = width;
	lightview.Width = height;
	lightview.MaxDepth = 1.0f;
	lightview.MinDepth = 0.0f;
	lightview.TopLeftX = 0;
	lightview.TopLeftY = 0;

	// Set position of light

	Lightpos = D3DXVECTOR3(0.0f, 10.0f, -100.0f);
	Lightat = D3DXVECTOR3(0.0f, 0.0f, -0.0f);

	D3DXQUATERNION quat;
	D3DXVECTOR3 vUp(0, 1, 0);

	D3DXMatrixLookAtLH(&Lightm, &Lightpos, &Lightat, &vUp);
	
	return hr;
}

HRESULT ShadowMap::RenderShadowMapSetup(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext, D3DXMATRIX& mWorld, D3DXMATRIX& mProj, D3DXMATRIX& mView)
{
	HRESULT hr;
	
	// set render targets for the stencil
	pd3dDeviceContext->ClearDepthStencilView(stencilviewSV, D3D11_CLEAR_DEPTH, 1.0, 0);
	ID3D11RenderTargetView* nullview = NULL;
	ID3D11RenderTargetView* prtvBackBuffer = DXUTGetD3D11RenderTargetView();
	pd3dDeviceContext->OMSetRenderTargets(1, &nullview, stencilviewSV);
	pd3dDeviceContext->RSSetViewports(1, &lightview);
	
	
	pd3dDeviceContext->RSSetState(shadowRS);

	//D3DXMatrixTranslation(&m, 0, 100, 0);
	//tmp *= m;
	
	dxmatWorldViewProjection = mWorld * Lightm * mProj;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	V(pd3dDeviceContext->Map(globalshadowbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	StencilCB* gsCB = (StencilCB*)MappedResource.pData;
	D3DXMatrixTranspose(&gsCB->worldviewproj, &dxmatWorldViewProjection);
	pd3dDeviceContext->Unmap(globalshadowbuffer, 0);
	pd3dDeviceContext->VSSetConstantBuffers(0, 1, &globalshadowbuffer);

	pd3dDeviceContext->IASetInputLayout(shadowvertexlayout);

	// No pixel shader as only depth buffer is being written
	pd3dDeviceContext->VSSetShader(shadowvertexshader, NULL, 0);
	pd3dDeviceContext->PSSetShader(NULL, NULL, 0); //terrain_PixelShader
	pd3dDeviceContext->GSSetShader(NULL, NULL, 0);

	/*
	Render Scene
	*/

	return hr;
}


void EndShadowMapRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext)
{
	ID3D11RenderTargetView* nullview = NULL;

	//pd3dDeviceContext->RSSetState(NULL);
	pd3dDeviceContext->OMSetRenderTargets(1, &nullview, NULL);

	pd3dDeviceContext->RSSetState(regsceneRS);

	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	vp.Height = (FLOAT)DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	ID3D11RenderTargetView* prtvBackBuffer = DXUTGetD3D11RenderTargetView();
	ID3D11DepthStencilView* pdsvBackBuffer = DXUTGetD3D11DepthStencilView();

	pd3dDeviceContext->OMSetRenderTargets(1, &prtvBackBuffer, pdsvBackBuffer);
	pd3dDeviceContext->RSSetViewports(1, &vp);
	//pd3dDeviceContext->IASetInputLayout(m_pVertexLayoutMesh);
}

HRESULT ShadowMap::SetupShadow(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext)
{
	HRESULT hr = S_OK;

	D3D11_MAPPED_SUBRESOURCE MappedResource;

	pd3dDeviceContext->VSSetShaderResources(0, 1, &stencilviewSR);
	pd3dDeviceContext->PSSetShaderResources(0, 1, &stencilviewSR);
	
	V(pd3dDeviceContext->Map(globalshadowgenbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	ShadowCB* pVSPerObject = (ShadowCB*)MappedResource.pData;
	D3DXMatrixTranspose(&pVSPerObject->lightviewproj, &dxmatWorldViewProjection);
	pVSPerObject->lightpos = Lightpos;
	pVSPerObject->shadowbias = bias;
	pd3dDeviceContext->Unmap(globalshadowgenbuffer, 0);

	pd3dDeviceContext->VSSetConstantBuffers(0, 1, &globalshadowgenbuffer);
	pd3dDeviceContext->PSSetConstantBuffers(0, 1, &globalshadowgenbuffer);

	V(pd3dDeviceContext->Map(globaltexturegenbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	ShadowTexCB* shadowtcb = (ShadowTexCB*)MappedResource.pData;
	shadowtcb->width = width;
	shadowtcb->height = height;
	pd3dDeviceContext->Unmap(globaltexturegenbuffer, 0);

	pd3dDeviceContext->VSSetConstantBuffers(1, 1, &globaltexturegenbuffer);
	pd3dDeviceContext->PSSetConstantBuffers(1, 1, &globaltexturegenbuffer);

	// Setup Samplers

	SetSamplers(pd3dDevice, pd3dDeviceContext);
	
	return hr;
}