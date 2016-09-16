#include "Terrain.h"
#include "SquareGrid.h"
#include <vector>

#define GRIDSIZE D3DXVECTOR3(100.f, 100.f, 120.f)
#define STARTPOS D3DXVECTOR3(-60.f, -20.f, -50.f)

ID3D11InputLayout* terrain_VertexLayout = NULL;

ID3D11VertexShader* terrain_VertexShader = NULL;
ID3D11PixelShader* terrain_PixelShader = NULL;

ID3D11Buffer* worldposCB = NULL;
ID3D11Buffer* materialCB = NULL;
ID3D11Buffer* raytraceCB = NULL;
ID3D11Buffer* squaregridCB = NULL;

ID3D11Texture2D* squaregridCB2D = NULL;

D3DXVECTOR3 lightpos;
float lightradius;
float numshadowrays;

struct WorldPosCB
{
	D3DXMATRIX worldviewprojection;
	D3DXMATRIX world;
};

struct MaterialCB
{
	D3DXVECTOR4 ambient;
	D3DXVECTOR4 diffuse;
	D3DXVECTOR4 specular;
	FLOAT shininess;
	BOOL texturei;
};

struct RayTraceDataCB
{
	D3DXVECTOR3 lightpos;
	FLOAT lightradius;
	D3DXVECTOR3 campos;
	FLOAT numshadowrays;
	FLOAT numindices;
};

struct SquareGridCB
{
	D3DXVECTOR3 lwh;
	FLOAT size;
	D3DXVECTOR3 start;
	FLOAT maxnum;
	FLOAT numperlink;
};

struct VertexUpload
{
	D3DXVECTOR3 position;
};

struct NormalUpload
{
	D3DXVECTOR3 normal;
};

struct TexCoord
{
	D3DXVECTOR2 uv;
};

Terrain::Terrain()
{

}

Terrain::~Terrain()
{

}

HRESULT Create_Terrain_Shader()
{
	HRESULT hr;

	ID3D11Device* pd3dDevice = DXUTGetD3D11Device();
	
	// Compile the shaders using the lowest possible profile for broadest feature level support
	
	string dataVS;

#ifdef _DEBUG
	
#ifdef x64

	ReadCSO("x64/Debug/RayTraceShaderVS.cso", dataVS);

	string dataPS;
	ReadCSO("x64/Debug/RayTraceShaderPS.cso", dataPS);

#else
	
#ifdef x32

	ReadCSO("Debug/RayTraceShaderVS.cso", dataVS);

	string dataPS;
	ReadCSO("Debug/RayTraceShaderPS.cso", dataPS);

#endif

#endif

#else

#ifdef x64

	ReadCSO("x64/Release/RayTraceShaderVS.cso", dataVS);

	string dataPS;
	ReadCSO("x64/Release/RayTraceShaderPS.cso", dataPS);

#else

#ifdef x32

	ReadCSO("Release/RayTraceShaderVS.cso", dataVS);

	string dataPS;
	ReadCSO("Release/RayTraceShaderPS.cso", dataPS);

#endif

#endif

#endif
	

	// Create the shaders
	V_RETURN(pd3dDevice->CreateVertexShader((void*)dataVS.c_str(),
		dataVS.size(), NULL, &terrain_VertexShader));
	
	DXUT_SetDebugName(terrain_VertexShader, "RayTraceVS");
	
	V_RETURN(pd3dDevice->CreatePixelShader((void*)dataPS.c_str(),
		dataPS.size(), NULL, &terrain_PixelShader));

	DXUT_SetDebugName(terrain_PixelShader, "RayTracePS");

	// Create our vertex input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	V_RETURN(pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), (void*)dataVS.c_str(),
		dataVS.size(), &terrain_VertexLayout));
	DXUT_SetDebugName(terrain_VertexLayout, "Primary");

	// Setup constant buffers
	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	
	Desc.ByteWidth = ComputeCBufferSize(sizeof(WorldPosCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &worldposCB));
	DXUT_SetDebugName(worldposCB, "WorldCB");
	
	Desc.ByteWidth = ComputeCBufferSize(sizeof(MaterialCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &materialCB));
	DXUT_SetDebugName(materialCB, "MaterialCB");
	
	Desc.ByteWidth = ComputeCBufferSize(sizeof(RayTraceDataCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &raytraceCB));
	DXUT_SetDebugName(raytraceCB, "RayTraceCB");
	
	Desc.ByteWidth = ComputeCBufferSize(sizeof(SquareGridCB));
	V_RETURN(pd3dDevice->CreateBuffer(&Desc, NULL, &squaregridCB));
	DXUT_SetDebugName(squaregridCB, "SquareGridCB");
	
	// Setup lighting

	lightpos = D3DXVECTOR3(0.0f, 0.0f, -10.0f);

	lightradius = 2.5f;

	numshadowrays = 20.f;

	return S_OK;
}

D3DXVECTOR3 getLightPos()
{
	return lightpos;
}

void setLightPos(D3DXVECTOR3 lightposp)
{
	lightpos = lightposp;
}

HRESULT Terrain::Load(char* filename)
{
	HRESULT hr;

	WCHAR str[260];

	// Load the mesh

	V(mesh.Load(filename, DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext()));

	// Change triangles into square grid

	squareGrid.createGrid(STARTPOS, 1.f, GRIDSIZE.x, GRIDSIZE.y, GRIDSIZE.z);

	D3DXVECTOR3 tri[3];

	for (int i = 0; i < mesh.getNumIndices(); i+=3)
	{
		if (i % 100 == 0)
			printf("%i\n", i);

		for (int j = 0; j < 3; j++)
			tri[j] = mesh.getVertices()[mesh.getIndices()[i + j]].position;

		squareGrid.triangletoBox(tri, i);
	}

	printf("Finished\n");
	
	//printf("%i\n", squareGrid.getSquareGridSize());
	// change mesh format for upload

	// get verts only (TODO: Add Material)

	// create vertex and normal arrays

	VertexUpload* vu = new VertexUpload[mesh.getNumVertices()];

	for (int i = 0; i < mesh.getNumVertices(); i++)
		vu[i].position = mesh.getVertices()[i].position;

	NormalUpload* nu = new NormalUpload[mesh.getNumVertices()];

	for (int i = 0; i < mesh.getNumVertices(); i++)
		nu[i].normal = mesh.getVertices()[i].normal;
	
	TexCoord* tc = new TexCoord[mesh.getNumVertices()];

	for (int i = 0; i < mesh.getNumVertices(); i++)
		tc[i].uv = mesh.getVertices()[i].texcoord;

	// setup data for vertex upload

	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = (void*)vu;
	data.SysMemPitch = 0;
	data.SysMemSlicePitch = 0;
	
	D3D11_TEXTURE1D_DESC desc;
	desc.Width = mesh.getNumVertices();
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;//DXGI_FORMAT_R8G8_B8G8_UNORM
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC descv;
	descv.Format = desc.Format;
	descv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	descv.Texture1D.MostDetailedMip = 0;
	descv.Texture1D.MipLevels = desc.MipLevels;
	
	ID3D11Texture1D* mesh_image = nullptr; // the verts
	ID3D11Texture1D* normal_image = nullptr; // the normals
	ID3D11Texture1D* texcoord_image = nullptr; // the texcoords
	ID3D11Texture1D* indices_image = nullptr; // the indices
	ID3D11Texture1D* squaregrid_image = nullptr; // the squaregrid
	ID3D11Texture1D* squaregrid2D_image = nullptr; // the squaregrid 2D
	ID3D11Texture1D* random_image = nullptr; // random numbers
	
	// create the verts

	V_RETURN(DXUTGetD3D11Device()->CreateTexture1D(&desc, &data, &mesh_image));

	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(mesh_image, &descv, &mesh1D));

	// setup data for normal upload

	data.pSysMem = (void*)nu;

	// create the normals

	V_RETURN(DXUTGetD3D11Device()->CreateTexture1D(&desc, &data, &normal_image));

	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(normal_image, &descv, &normal1D));
	
	// setup data for texcoord upload
	
	data.pSysMem = (void*)tc;
	desc.Format = DXGI_FORMAT_R32G32_FLOAT;

	descv.Format = desc.Format;

	// create the texcoord

	V_RETURN(DXUTGetD3D11Device()->CreateTexture1D(&desc, &data, &texcoord_image));

	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(texcoord_image, &descv, &texcoord1D));

	// create random vars

	const int randomsNum = 1000;
	float randoms[randomsNum];

	for (int i = 0; i < randomsNum; i++)
		randoms[i] = rand() % 1000 / 1000.f;

	data.pSysMem = (void*)randoms;
	desc.Format = DXGI_FORMAT_R32_FLOAT;

	desc.Width = randomsNum;
	descv.Format = desc.Format;

	V_RETURN(DXUTGetD3D11Device()->CreateTexture1D(&desc, &data, &random_image));

	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(random_image, &descv, &random1D));

	// create the indices

	data.pSysMem = (void*)mesh.getIndices();
	
	desc.Width = mesh.getNumIndices();
	desc.Format = DXGI_FORMAT_R32_UINT;
	
	descv.Format = desc.Format;
	
	V_RETURN(DXUTGetD3D11Device()->CreateTexture1D(&desc, &data, &indices_image));

	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(indices_image, &descv, &indices1D));
	
	// create the square grid 2D texture
	
	// find the size of the grid

	double gridsqrt = sqrt(squareGrid.getGridSize());

	unsigned int w = floor(gridsqrt), h = ceil(gridsqrt);

	// resize the map to fit the new dimensions

	squareGrid.resize(w * h);

	D3D11_TEXTURE2D_DESC desc2D;
	desc2D.Width = w;
	desc2D.Height = h;
	desc2D.MipLevels = 1;
	desc2D.ArraySize = 1;

	desc2D.SampleDesc.Count = 1;
	desc2D.SampleDesc.Quality = 0;
	desc2D.Usage = D3D11_USAGE_DEFAULT;
	desc2D.Format = DXGI_FORMAT_R32_UINT;
	desc2D.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	desc2D.CPUAccessFlags = 0;
	desc2D.MiscFlags = 0;
	
	D3D11_SUBRESOURCE_DATA data2D;
	data2D.pSysMem = (void*)squareGrid.getGrid();
	data2D.SysMemPitch = w * 4;
	data2D.SysMemSlicePitch = h * 4;
	
	D3D11_SHADER_RESOURCE_VIEW_DESC descv2D;
	descv2D.Format = desc.Format;
	descv2D.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	descv2D.Texture1D.MostDetailedMip = 0;
	descv2D.Texture1D.MipLevels = desc.MipLevels;
	
	V_RETURN(DXUTGetD3D11Device()->CreateTexture2D(&desc2D, &data2D, &squaregridCB2D));
	
	V_RETURN(DXUTGetD3D11Device()->CreateShaderResourceView(squaregridCB2D, &descv2D, &squaregrid2D));



	// load an image

	V_RETURN(DXUTGetGlobalResourceCache().CreateTextureFromFile(DXUTGetD3D11Device(), DXUTGetD3D11DeviceContext(), L"rocks.jpg", &diffuse_map, false));
	
	// get rid of tmp verts

	free(vu);
	
	return S_OK;
}

HRESULT Terrain::Draw(ID3D11DeviceContext* pd3dImmediateContext, D3DXVECTOR3& vLightDir, D3DXMATRIX& mWorld, D3DXMATRIX& mProj, D3DXMATRIX& mView, const D3DXVECTOR3* eyept)
{
	HRESULT hr;

	//Get the mesh
	//IA setup
	pd3dImmediateContext->IASetInputLayout(terrain_VertexLayout);
	UINT Strides[1];
	UINT Offsets[1];
	ID3D11Buffer* pVB[1];
	pVB[0] = mesh.getVertexBuffer();
	Strides[0] = mesh.getVertexStride();
	Offsets[0] = 0;
	
	pd3dImmediateContext->IASetVertexBuffers(0, 1, pVB, Strides, Offsets);
	pd3dImmediateContext->IASetIndexBuffer(mesh.getIndexBuffer(), mesh.getIndexFormat(), 0);

	// Set the shaders
	pd3dImmediateContext->VSSetShader(terrain_VertexShader, NULL, 0);
	pd3dImmediateContext->PSSetShader(terrain_PixelShader, NULL, 0);
	
	D3DXMATRIX mWorldViewProjection = mWorld * mView * mProj;

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	
	V(pd3dImmediateContext->Map(worldposCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	WorldPosCB* wpCB = (WorldPosCB*)MappedResource.pData;

	D3DXMatrixTranspose(&wpCB->worldviewprojection, &mWorldViewProjection);
	D3DXMatrixTranspose(&wpCB->world, &mWorld);
	pd3dImmediateContext->Unmap(worldposCB, 0);

	pd3dImmediateContext->VSSetConstantBuffers(0, 1, &worldposCB);
	pd3dImmediateContext->PSSetConstantBuffers(0, 1, &worldposCB);
	
	// setup texture 1D mesh array

	pd3dImmediateContext->PSSetShaderResources(1, 1, &mesh1D);
	pd3dImmediateContext->PSSetShaderResources(2, 1, &normal1D);
	pd3dImmediateContext->PSSetShaderResources(3, 1, &texcoord1D);
	pd3dImmediateContext->PSSetShaderResources(4, 1, &indices1D);
	pd3dImmediateContext->PSSetShaderResources(5, 1, &squaregrid2D);
	pd3dImmediateContext->PSSetShaderResources(6, 1, &random1D);

	// map the light
	
	V(pd3dImmediateContext->Map(raytraceCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	RayTraceDataCB* rtCB = (RayTraceDataCB*)MappedResource.pData;

	rtCB->campos = *eyept;
	rtCB->lightpos = lightpos;
	rtCB->lightradius = lightradius;
	rtCB->numshadowrays = numshadowrays;
	rtCB->numindices = mesh.getNumIndices();
	pd3dImmediateContext->Unmap(raytraceCB, 0);
	
	pd3dImmediateContext->PSSetConstantBuffers(2, 1, &raytraceCB);
	
	// map the squaregrid
	
	V(pd3dImmediateContext->Map(squaregridCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
	SquareGridCB* sgCB = (SquareGridCB*)MappedResource.pData;

	sgCB->lwh = GRIDSIZE;
	sgCB->size = squareGrid.getSize();
	sgCB->start = *squareGrid.getStart();
	sgCB->maxnum = squareGrid.getMaxNum();
	sgCB->numperlink = squareGrid.getNumPerLink();
	
	pd3dImmediateContext->Unmap(squaregridCB, 0);

	pd3dImmediateContext->PSSetConstantBuffers(3, 1, &squaregridCB);
	

	//Render
	
	D3D11_PRIMITIVE_TOPOLOGY PrimType;

	Material* mat;
	UINT subset = 0;
	//for (UINT subset = 0; subset < mesh.getNumMesh(); ++subset)
	{
		// Get the subset
		
		PrimType = mesh.getPrimativeType();
		pd3dImmediateContext->IASetPrimitiveTopology(PrimType);

		V(pd3dImmediateContext->Map(materialCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource));
		MaterialCB* mCB = (MaterialCB*)MappedResource.pData;

		// get material

		mat = mesh.getMaterial(subset);

		mCB->ambient = mat->ambient;
		mCB->diffuse = mat->diffuse;
		mCB->specular = mat->specular;
		mCB->shininess = mat->shininess;
		
		// TODO: D3D11 - material loading
		ID3D11ShaderResourceView* pDiffuseRV = mat->pTextureRV11;

		if (pDiffuseRV == NULL)
			mCB->texturei = false;
		else
		{
			mCB->texturei = true;

			pd3dImmediateContext->PSSetShaderResources(0, 1, &pDiffuseRV);
		}

		pd3dImmediateContext->Unmap(materialCB, 0);

		pd3dImmediateContext->VSSetConstantBuffers(1, 1, &materialCB);
		pd3dImmediateContext->PSSetConstantBuffers(1, 1, &materialCB);

		// TMP
		
		pd3dImmediateContext->DrawIndexed(mesh.getNumIndices(), 0, 0);
		//pd3dImmediateContext->DrawIndexed((UINT)pSubset->IndexCount, (UINT)pSubset->IndexStart, (UINT)pSubset->VertexStart);
	}
	//exit(-1);
	return hr;
}

void Terrain::DrawNoTexture(ID3D11DeviceContext* pd3dImmediateContext)
{
	//terrain_mesh.Render(pd3dImmediateContext, 0, 1);
}