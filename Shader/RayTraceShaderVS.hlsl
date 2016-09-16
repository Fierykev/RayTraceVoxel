cbuffer StandardShaderCB : register(b0)
{
	matrix worldviewprojection : packoffset(c0);
	matrix world : packoffset(c4);
};

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
	float4 worldpos : TEXCOORD1;
};

PSINPUT mainVS(VSINPUT In)
{
	PSINPUT O;

	O.position = mul(In.position, worldviewprojection);
	O.normal = In.normal;// mul(In.normal, (float3x3)world);
	O.texcoord = In.texcoord;
	O.worldpos = In.position;// mul(In.position, world);

	return O;
}