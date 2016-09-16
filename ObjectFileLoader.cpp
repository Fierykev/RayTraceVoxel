#include "DXUT.h"
#include "SDKmisc.h"
#include "ObjectFileLoader.h" // link to the header
#include "Shader.h"

#include "DXUT.h"
#include "SDKMesh.h"
#include "SDKMisc.h"

/***************************************************************************
OBJ Loading
***************************************************************************/

ObjLoader::ObjLoader()
{
	vertex_final_array = nullptr;
}

void ObjLoader::freeOnStack()
{
	if (vertex_final_array != nullptr)
		free(vertex_final_array);
}

ObjLoader::~ObjLoader()
{
	// delete all data

	freeOnStack();
	const int ERROR_VALUE = 1;
	/*
	for (unsigned int i = 0; i < material->size(); i++)
	{
	Material *pMaterial = material.at(i);

	if (pMaterial->pTextureRV10 && pMaterial->pTextureRV10 == (ID3D10ShaderResourceView*)ERROR_VALUE)
	{
	ID3D10Resource* pRes = NULL;

	pMaterial->pTextureRV10->GetResource( &pRes );
	SAFE_RELEASE( pRes );
	SAFE_RELEASE( pRes );   // do this twice, because GetResource adds a ref

	SAFE_RELEASE( pMaterial->pTextureRV10 );
	}

	SAFE_DELETE( pMaterial );
	}*/

	// deallocate all of the vectors

	//delete vx_array_i;
	/*
	delete vx_array;

	delete vertex_final_array;

	delete material;

	delete attributes;

	delete mesh_num;

	SAFE_DELETE_ARRAY(m_pAttribTable);



	SAFE_RELEASE(mesh);*/
}

void ObjLoader::Base_Mat(Material *mat)
{
	ZeroMemory(mat, sizeof(mat)); // allocate memory

	mat->ambient = D3DXVECTOR4(0.2f, 0.2f, 0.2f, 1.f);
	mat->diffuse = D3DXVECTOR4(0.8f, 0.8f, 0.8f, 1.f);
	mat->specular = D3DXVECTOR4(1.0f, 1.0f, 1.0f, 1.f);
	mat->shininess = 0;
	mat->alpha = 1.0f;
	mat->specularb = false;
	mat->pTextureRV11 = NULL;
}

void ObjLoader::Material_File(string filename, string matfile, unsigned long* tex_num, ID3D11Device* Device)
{
	// find the directory to the material file

	string directory = filename.substr(0, filename.find_last_of('/') + 1);

	matfile = directory + matfile; // the location of the material file to the program

	// open the file

	ifstream matFile_2(matfile);

	if (matFile_2.is_open()) // If obj file is open, continue
	{
		string line_material;// store each line of the file here

		while (!matFile_2.eof()) // Start reading file data as long as we have not reached the end
		{
			getline(matFile_2, line_material); // Get a line from file

			// convert to a char to do pointer arithmetics

			char* ptr = (char*)line_material.c_str();

			if (ptr[0] == '	')// this is tab not space (3ds max uses tabs which would otherwise confuse this program without this line)
			{
				ptr++;// move address up
			}

			// This program is for standard Wavefront Objects that are triangulated and have normals stored in the file.  This reader has been tested with 3ds Max and Blender.

			if (ptr[0] == 'n' && ptr[1] == 'e' && ptr[2] == 'w' && ptr[3] == 'm'
				&& ptr[4] == 't' && ptr[5] == 'l') // new material
			{
				ptr += 6 + 1;// move address up

				Material mat; // allocate memory to create a new material

				Base_Mat(&mat); // init the material

				mat.name = ptr; // set the name of the material

				material.push_back(mat); // add to the vector

				*tex_num = material.size() - 1;
			}
			else if (ptr[0] == 'K' && ptr[1] == 'a') // ambient
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).ambient[0],
					&material.at(*tex_num).ambient[1],
					&material.at(*tex_num).ambient[2]);

				material.at(*tex_num).ambient[3] = 1.f;
			}
			else if (ptr[0] == 'K' && ptr[1] == 'd') // diffuse
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).diffuse[0],
					&material.at(*tex_num).diffuse[1],
					&material.at(*tex_num).diffuse[2]);

				material.at(*tex_num).diffuse[3] = 1.f;
			}
			else if (ptr[0] == 'K' && ptr[1] == 's') // specular
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f %f %f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).specular[0],
					&material.at(*tex_num).specular[1],
					&material.at(*tex_num).specular[2]);

				material.at(*tex_num).specular[3] = 1.f;
			}
			else if (ptr[0] == 'N' && ptr[1] == 's') // shininess
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).shininess);
			}
			else if (ptr[0] == 'd') // transparency
			{
				ptr++;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).alpha);
			}
			else if (ptr[0] == 'T' && ptr[0] == 'r') // another way to store transparency
			{
				ptr += 2;// move address up

				sscanf_s(ptr, "%f ",							// Read floats from the line: v X Y Z
					&material.at(*tex_num).alpha);
			}
			else if (ptr[0] == 'm' && ptr[1] == 'a' && ptr[2] == 'p' && ptr[3] == '_'
				&& ptr[4] == 'K' && ptr[5] == 'd') // image texture
			{
				ptr += 7;// move address up

				material.at(*tex_num).texture_path = directory + ptr; // the material file

				// load the file

				// convert to a LPWSTR

				wstring wstr_;
				wstr_.assign(material.at(*tex_num).texture_path.begin(), material.at(*tex_num).texture_path.end());
				LPWSTR LPWSTR_ = const_cast<LPWSTR>(wstr_.c_str());

				if (FAILED(D3DX11CreateShaderResourceViewFromFile(Device, LPWSTR_, NULL, NULL, &material.at(*tex_num).pTextureRV11, NULL))) // create the texture
				{
					printf("Error (OBJECT LOADER): Cannot Load Image File- %s\n", material.at(*tex_num).texture_path.c_str());
				}
			}
		}

		matFile_2.close(); // close the file
	}
	else
	{
		printf("Error (OBJECT LOADER): Cannot Find Material File- %s\n", matfile.c_str());;
	}
}

void ObjLoader::Load_Geometry(char *filename, ID3D11Device* Device)
{
	// delete past memory

	freeOnStack();

	// allocate memory to the vectors on the heap

	vx_array_i.clear();

	vx_array.clear();

	material.clear();

	attributes.clear();

	mesh_num = 0;

	// create maps to store the lighting values for the material

	ifstream objFile(filename); // open the object file

	if (objFile.is_open()) // If the obj file is open, continue
	{
		// initialize the strings needed to read the file

		string line;

		string mat;

		// the material that is used

		unsigned long material_num = 0;

		unsigned long tex_num = 0;

		numVerts = 0;

		// Store the coordinates

		vector <float> vn_array;

		vector <float> vt_array;

		while (!objFile.eof()) // start reading file data
		{
			getline(objFile, line);	// get line from file

			// convert to a char to do pointers

			const char* ptr = line.c_str();

			if (ptr[0] == 'm' && ptr[1] == 't' && ptr[2] == 'l' && ptr[3] == 'l'  && ptr[4] == 'i' && ptr[5] == 'b' && ptr[6] == ' ') // load the material file
			{
				ptr += 7; // move the address up

				const string material_file = ptr;// the material file

				Material_File(filename, material_file, &tex_num, Device); // read the material file and update the number of materials
			}
			if (ptr[0] == 'v' && ptr[1] == ' ') // the first character is a v: on this line is a vertex stored.
			{
				ptr += 2; // move address up

				// store the three tmp's into the verticies

				float tmp[3];

				sscanf_s(ptr, "%f %f %f ", // read floats from the line: X Y Z
					&tmp[0],
					&tmp[1],
					&tmp[2]);

				vx_array.push_back(tmp[0]);
				vx_array.push_back(tmp[1]);
				vx_array.push_back(tmp[2]);
			}

			else if (ptr[0] == 'v' && ptr[1] == 'n') // the vertex normal
			{
				ptr += 2;

				// store the three tmp's into the verticies

				float tmp[3];

				sscanf_s(ptr, "%f %f %f ", // read floats from the line: X Y Z
					&tmp[0],
					&tmp[1],
					&tmp[2]);

				vn_array.push_back(tmp[0]);
				vn_array.push_back(tmp[1]);
				vn_array.push_back(tmp[2]);
			}

			else if (ptr[0] == 'v' && ptr[1] == 't') // texture coordinate for a vertex
			{
				ptr += 2;

				// store the two tmp's into the verticies

				float tmp[2];

				sscanf_s(ptr, "%f %f ",	// read floats from the line: X Y Z
					&tmp[0],
					&tmp[1]);

				vt_array.push_back(tmp[0]);
				vt_array.push_back(tmp[1]);
			}
			else if (ptr[0] == 'u' && ptr[1] == 's' && ptr[2] == 'e' && ptr[3] == 'm' && ptr[4] == 't' && ptr[5] == 'l') // which material is being used
			{
				mat = line.substr(6 + 1, line.length());// save so the comparison will work
				
				// add new to the material name so that it matches the names of the materials in the mtl file

				for (unsigned long num = 0; num < tex_num + 1; num++)// find the material
				{
					if (mat == material.at(num).name)// matches material in mtl file
					{
						material_num = num;
					}
				}
			}
			else if (ptr[0] == 'f') // store the faces in the object
			{
				ptr++;

				int vertexNumber[3] = { 0, 0, 0 };
				int normalNumber[3] = { 0, 0, 0 };
				int textureNumber[3] = { 0, 0, 0 };

				sscanf_s(ptr, "%i/%i/%i %i/%i/%i %i/%i/%i ",
					&vertexNumber[0],
					&textureNumber[0],
					&normalNumber[0],
					&vertexNumber[1],
					&textureNumber[1],
					&normalNumber[1],
					&vertexNumber[2],
					&textureNumber[2],
					&normalNumber[2]
					); // each point represents an X,Y,Z.

				// create a vertex for this area

				for (int i = 0; 3 > i; i++) // loop for each triangle
				{
					Vertex vert;

					vert.position = D3DXVECTOR3(vx_array.at((vertexNumber[i] - 1) * 3), vx_array.at((vertexNumber[i] - 1) * 3 + 1), vx_array.at((vertexNumber[i] - 1) * 3 + 2));

					vert.normal = D3DXVECTOR3(vn_array[(normalNumber[i] - 1) * 3], vn_array[(normalNumber[i] - 1) * 3 + 1], vn_array[(normalNumber[i] - 1) * 3 + 2]);

					vert.texcoord = D3DXVECTOR2(vt_array[(textureNumber[i] - 1) * 2], vt_array[(textureNumber[i] - 1) * 2 + 1]);

					unsigned int index = 0;

					bool indexupdate = false;

					if (vertexmap.find(vert.position) != vertexmap.end())
						for (VertexDataforMap vdm : vertexmap[vert.position])
						{
							if (vert.normal == vdm.normal && vert.texcoord == vdm.texcoord) // found the index
							{
								index = vdm.index;

								indexupdate = true;
								break;
							}
						}

					// nothing found

					if (!indexupdate)
					{
						VertexDataforMap tmp;

						index = numVerts;

						tmp.normal = vert.normal;

						tmp.texcoord = vert.texcoord;

						tmp.index = index;

						vertexmap[vert.position].push_back(tmp);

						numVerts ++;
					}

					vx_array_i.push_back(index);
				}

				// add the texture number to the attributes

				attributes.push_back(material_num);
			}
		}

		// create the final verts

		Vertex vert;

		vertex_final_array = new Vertex[numVerts];

		for (map<D3DXVECTOR3, vector<VertexDataforMap>>::iterator i = vertexmap.begin(); i != vertexmap.end(); i++)
		{
			for (VertexDataforMap vdm : i->second)
			{
				vertex_final_array[vdm.index].position = i->first;

				vertex_final_array[vdm.index].normal = vdm.normal;

				vertex_final_array[vdm.index].texcoord = vdm.texcoord;
			}
		}
	}
	else
	{
		printf("Error (OBJECT LOADER):  Cannot Find Object File- %s\n", filename);
	}
}

HRESULT ObjLoader::Load(char *filename, ID3D11Device* Device, ID3D11DeviceContext* Context)
{
	HRESULT hr;
	
	Load_Geometry(filename, Device);

	// Load the materials
	const int ERROR_VALUE = 1;

	for (unsigned long i = 0; i < material.size(); i++)
	{
		Material *pMaterial = &material.at(i);
		if (pMaterial->texture_path[0] != '\0') // this array holds data
		{
			pMaterial->pTextureRV11 = (ID3D11ShaderResourceView*)ERROR_VALUE; // it equals to an error value

			// TODO: Check if the file exists first
			{
				// convert string to WCHAR

				wstring wstr_;
				wstr_.assign(pMaterial->texture_path.begin(), pMaterial->texture_path.end());
				LPWSTR LPWSTR_ = const_cast<LPWSTR>(wstr_.c_str());

				DXUTGetGlobalResourceCache().CreateTextureFromFile(Device, Context, LPWSTR_, &pMaterial->pTextureRV11, false);
			}
		}
	}

	// Now let's place the object mesh into the buffers structure

	// Setup vertex buffer

	D3D11_BUFFER_DESC Desc;
	Desc.Usage = D3D11_USAGE_DEFAULT;
	Desc.ByteWidth = sizeof(Vertex) * getNumVertices();
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;

	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = (void*)vertex_final_array;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	V(Device->CreateBuffer(&Desc, &InitData, &mesh_verts));

	// Setup index buffer

	Desc.ByteWidth = sizeof(unsigned int) * getNumIndices();
	Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	
	InitData.pSysMem = (void*)&vx_array_i.at(0);

	V(Device->CreateBuffer(&Desc, &InitData, &mesh_indices));

	return S_OK;
}

/*

void ObjLoader::RenderSubset(unsigned int iSubset, ID3D10EffectShaderResourceVariable* texture)
{
HRESULT hr;

Material* pMaterial = getSubMaterail( iSubset );

// add in the materials and light reactions here

V( g_pAmbient->SetFloatVector(pMaterial->ambient));
V( g_pDiffuse->SetFloatVector(pMaterial->diffuse));
V( g_pSpecular->SetFloatVector(pMaterial->specular));
V( g_pOpacity->SetFloat(pMaterial->alpha));
V( g_pSpecularPower->SetInt(pMaterial->shininess ));

if ( !IsErrorResource( pMaterial->pTextureRV10 ) )
texture->SetResource( pMaterial->pTextureRV10 );

D3D10_TECHNIQUE_DESC techDesc;
pMaterial->pTechnique->GetDesc( &techDesc );

for ( UINT p = 0; p < techDesc.Passes; ++p )
{
pMaterial->pTechnique->GetPassByIndex(p)->Apply(0);
getMesh()->DrawSubset(iSubset);
}
}

void ObjLoader::Draw(ID3D10EffectShaderResourceVariable* texture)
{
// loop by the number of textures that there are

//
// Set the Vertex Layout
//

for ( UINT iSubset = 0; iSubset < getMesh_Num(); ++iSubset )
{
RenderSubset(iSubset, texture);
}
}*/