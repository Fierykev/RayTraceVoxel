cbuffer ShadowMapCB : register(b0)
{
	matrix worldviewprojection : packoffset(c0);
};

struct VSIN
{
	float4 position : POSITION;
};

struct VSOUT
{
	float4 position : SV_POSITION;
};

VSOUT mainVS(VSIN In)
{
	// Only output the position (depth)

	VSOUT O;
	O.position = mul(In.position, worldviewprojection);

	return O;
}