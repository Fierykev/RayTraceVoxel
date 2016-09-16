cbuffer cbPerObject : register(b0)
{
	matrix g_mWorldViewProjection    : packoffset(c0);
};

struct VS_INPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 texcoord : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position    : SV_POSITION;
};

VS_OUTPUT VSMain(VS_INPUT Input)
{
	VS_OUTPUT Output;

	// There is nothing special here, just transform and write out the depth.
	Output.position = mul(Input.position, g_mWorldViewProjection);

	return Output;
}