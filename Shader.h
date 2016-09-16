#ifndef SHADER_H
#define SHADER_H

#include <string>
#include "DXUT.h"

using namespace std;

unsigned int ComputeCBufferSize(double size);

HRESULT CompileShaderFromFile(WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut);

HRESULT SetSamplers(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3dDeviceContext);

bool ReadCSO(string filename, string& data);

#endif