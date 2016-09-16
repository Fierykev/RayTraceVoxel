#include "DXUT.h"
#include "DXUTcamera.h"

HRESULT LoadShadowShader(ID3D11Device* pd3dDevice);

void EndShadowMapRender(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext);

extern float bias;

class ShadowMap
{
public:

	ShadowMap();

	~ShadowMap();

	HRESULT CreateShadowShader(ID3D11Device* pd3dDevice);

	HRESULT RenderShadowMapSetup(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext, D3DXMATRIX& mWorld, D3DXMATRIX& mProj, D3DXMATRIX& mView);

	HRESULT SetupShadow(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext);

private:

	int width, height;

	ID3D11Texture2D* shadowmaptex;
	ID3D11DepthStencilView* stencilviewSV;
	ID3D11ShaderResourceView* stencilviewSR;
	D3D11_VIEWPORT lightview;
	D3DXMATRIXA16 Lightm;
	D3DXVECTOR3 Lightpos;
	D3DXVECTOR3 Lightat;
	D3DXMATRIX dxmatWorldViewProjection;
};