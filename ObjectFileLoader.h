#ifndef OBJECT_FILE_LOADER_H
#define OBJECT_FILE_LOADER_H
#pragma once

#include "DXUT.h"

// loading files

#include <fstream>

// get line in file

#include <sstream>

// Need these headers to support the array types I want

#include <map>
#include <vector>

#include <string>
#include <stdint.h>

// namespace time

using namespace std;// load all std:: things

struct Material
{
	std::string name;

	D3DXVECTOR4 ambient;

	D3DXVECTOR4 diffuse;

	D3DXVECTOR4 specular;

	int shininess;

	float alpha;

	bool specularb;

	std::string texture_path;

	// DirectX Specific

	ID3D11ShaderResourceView* pTextureRV11;
};

struct Vertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	D3DXVECTOR2 texcoord;
};

struct VertexDataforMap
{
	D3DXVECTOR3 normal;
	D3DXVECTOR2 texcoord;
	unsigned int index;
};

class ObjLoader
{
public:

	ObjLoader();

	~ObjLoader(); // destruction method

	HRESULT ObjLoader::Load(char *filename, ID3D11Device* Device, ID3D11DeviceContext* Context); // Load the object with its materials

	// get the number of materials used

	const unsigned int ObjLoader::getMat_Num()
	{
		return material.size();
	}

	// get the material pointer

	Material* ObjLoader::getMaterials()
	{
		return &material.at(0);
	}

	// get the number of vertices in the object

	const unsigned int ObjLoader::getNumVertices()
	{
		return numVerts;
	}


	// get the number of indices in the object

	const unsigned int ObjLoader::getNumIndices()
	{
		return vx_array_i.size();
	}

	// get a pointer to the verticies

	const Vertex* ObjLoader::getVertices()
	{
		return vertex_final_array;
	}

	// get a pointer to the vertex buffer

	ID3D11Buffer* ObjLoader::getVertexBuffer()
	{
		return mesh_verts;
	}

	// get the vertex stride

	UINT ObjLoader::getVertexStride()
	{
		return sizeof(Vertex);
	}

	// get a pointer to the indices

	const unsigned int* ObjLoader::getIndices()
	{
		return &vx_array_i.at(0);
	}

	// get a pointer to the index buffer

	ID3D11Buffer* ObjLoader::getIndexBuffer()
	{
		return mesh_indices;
	}

	// get the format of the indices

	DXGI_FORMAT ObjLoader::getIndexFormat()
	{
		return DXGI_FORMAT_R32_UINT;
	}

	// get the number of meshes used to draw the object

	const unsigned int ObjLoader::getNumMesh()
	{
		return mesh_num;
	}

	// get a pointer to a certain material

	Material* ObjLoader::getMaterial(unsigned int mat_num)
	{
		return &material.at(mat_num);
	}

	// get the primative type

	D3D11_PRIMITIVE_TOPOLOGY ObjLoader::getPrimativeType()
	{
		return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}

	void ObjLoader::freeOnStack();

private:

	// Create a vector to store the verticies

	void ObjLoader::Load_Geometry(char *filename, ID3D11Device* Device); // load the verticies and indices

	void ObjLoader::Material_File(string filename, string matfile, unsigned long* tex_num, ID3D11Device* Device); // load the material file

	void ObjLoader::Base_Mat(Material *mat); // the basic material

	void ObjLoader::RenderSubset(unsigned int iSubset, ID3D10EffectShaderResourceVariable* texture); // render a subset of the mesh

	// variables

	vector <unsigned int> vx_array_i; // store the indecies for the vertex

	vector <float> vx_array; // store the verticies in the mesh

	Vertex* vertex_final_array; // the final verticies organized for Direct3D to draw

	vector <Material> material; // the materials used on the object

	vector <unsigned long> attributes;

	map <D3DXVECTOR3, vector<VertexDataforMap>> vertexmap; // map for removing doubles

	unsigned int numVerts;

	// Mesh management

	ID3D11Buffer* mesh_verts; // our mesh	

	ID3D11Buffer* mesh_indices; // our indices

	unsigned int mesh_num; // the number of meshes
};

#endif