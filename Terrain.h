#ifndef TERRAIN_H
#define TERRAIN_H

#include "ObjectFileLoader.h"
#include "DXUT.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"

#include "Shader.h"
#include "SquareGrid.h"

extern ID3D11PixelShader* terrain_PixelShader;

class Terrain
{
public:

	Terrain();

	~Terrain();

	HRESULT Draw(ID3D11DeviceContext* pd3dImmediateContext, D3DXVECTOR3& vLightDir, D3DXMATRIX& mWorld, D3DXMATRIX& mProj, D3DXMATRIX& mView, const D3DXVECTOR3* eyept);

	HRESULT Load(char* filename);

	void Terrain::DrawNoTexture(ID3D11DeviceContext* pd3dImmediateContext);

private:

	ObjLoader mesh;

	SquareGrid squareGrid;

	ID3D11ShaderResourceView* diffuse_map;

	ID3D11ShaderResourceView* mesh1D;

	ID3D11ShaderResourceView* normal1D;

	ID3D11ShaderResourceView* texcoord1D;

	ID3D11ShaderResourceView* indices1D;

	ID3D11ShaderResourceView* squaregrid1D;

	ID3D11ShaderResourceView* squaregrid2D;

	ID3D11ShaderResourceView* random1D;
};

HRESULT Create_Terrain_Shader();

D3DXVECTOR3 getLightPos();

void setLightPos(D3DXVECTOR3 lightposp);

#endif