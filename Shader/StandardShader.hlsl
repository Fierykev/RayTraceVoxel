#include "Extensions/Sampler.hlsl"
#include "Extensions/Shadow.hlsl"

cbuffer StandardShaderCB : register(b2)
{
	matrix worldviewprojection : packoffset(c0);
	matrix world : packoffset(c4);
}

cbuffer PerObjectCB : register(b3)
{
	float4 ambient : packoffset(c0);
	float4 diffuse : packoffset(c1);
	bool texturei : packoffset(c2);
}

struct VSINPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
};

struct PSINPUT
{
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
	float4 position : SV_POSITION;
	float4 texshadow : TEXCOORD1;
};

Texture2D diffusemap : register(t1); // t0 is used by the shadow map

PSINPUT mainVS(VSINPUT In)
{
	PSINPUT O;

	O.position = mul(In.position, worldviewprojection);
	O.normal = mul(In.normal, (float3x3)world);
	O.texcoord = In.texcoord;

	O.texshadow = ShadowCalc(In.position, world);

	return O;
}

float4 mainPS(PSINPUT In) : SV_TARGET
{
	float4 diffuse_color = diffuse;

	if (texturei)
		diffuse_color *= diffusemap.Sample(linearsampler, In.texcoord);

	return ShadowColorCalc(diffuse_color, ambient, In.normal, In.texshadow);
}