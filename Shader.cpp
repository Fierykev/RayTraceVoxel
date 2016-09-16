#include "DXUT.h"
#include "SDKmisc.h"
#include <fstream>

using namespace std;

unsigned int ComputeCBufferSize(double size)
{
	double ratio = size / 16.0;

	return ceil(ratio) * 16;
}

HRESULT SetSamplers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext)
{
	HRESULT hr;

	ID3D11SamplerState* SamLinear = NULL;
	ID3D11SamplerState* SamCmp = NULL;
	ID3D11SamplerState* SamPoint = NULL;

	// Create a sampler state
	D3D11_SAMPLER_DESC SamDesc;
	SamDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamDesc.MipLODBias = 0.0f;
	SamDesc.MaxAnisotropy = 1;
	SamDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamDesc.BorderColor[0] = SamDesc.BorderColor[1] = SamDesc.BorderColor[2] = SamDesc.BorderColor[3] = 0;
	SamDesc.MinLOD = 0;
	SamDesc.MaxLOD = D3D11_FLOAT32_MAX;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &SamLinear));
	DXUT_SetDebugName(SamLinear, "Linear Sample");

	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	SamDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	SamDesc.ComparisonFunc = D3D11_COMPARISON_LESS;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &SamCmp));
	DXUT_SetDebugName(SamCmp, "Linear Sample Comparison");

	SamDesc.MaxAnisotropy = 15;
	SamDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	SamDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	SamDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	V_RETURN(pd3dDevice->CreateSamplerState(&SamDesc, &SamPoint));
	DXUT_SetDebugName(SamPoint, "Point Sample");

	// Set the samplers

	pd3dDeviceContext->PSSetSamplers(0, 1, &SamLinear);
	pd3dDeviceContext->PSSetSamplers(1, 1, &SamCmp);
	pd3dDeviceContext->PSSetSamplers(2, 1, &SamPoint);

	DeleteObject(SamLinear);
	DeleteObject(SamCmp);
	DeleteObject(SamPoint);
}

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	// find the file
	WCHAR str[MAX_PATH];
	V_RETURN(DXUTFindDXSDKMediaFileCch(str, MAX_PATH, szFileName));

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if defined( DEBUG ) || defined( _DEBUG )
	// Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
	// Setting this flag improves the shader debugging experience, but still allows 
	// the shaders to be optimized and to run exactly the way they will run in 
	// the release configuration of this program.
	dwShaderFlags |= D3DCOMPILE_DEBUG;
#endif

	ID3DBlob* pErrorBlob;
	hr = D3DX11CompileFromFile(str, NULL, NULL, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, NULL, ppBlobOut, &pErrorBlob, NULL);
	if (FAILED(hr))
	{
		if (pErrorBlob != NULL)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		SAFE_RELEASE(pErrorBlob);
		return hr;
	}
	SAFE_RELEASE(pErrorBlob);

	return S_OK;
}

bool ReadCSO(string filename, string& data)
{
	ifstream file(filename, ios::in | ios::binary);
	
	if (file.is_open())
		data.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	else
		return false;

	return true;
}