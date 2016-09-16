cbuffer ShadowData : register(b0)
{
	matrix g_mWorldViewProjection : packoffset(c0);
	matrix g_mWorld : packoffset(c4);
	//matrix lightviewproj : packoffset(c8);
	//float3 lightpos : packoffset(c12);
	//float shadowbias : packoffset(c12.a);
	//float shadowbiastmp : packoffset(c13);
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
	float4 texshadow : TEXCOORD1;
};

SamplerState pointsampler : register(s0);
//SamplerState linearsampler : register(s1);

//Texture2D diffusemap : register(t0);
Texture2D shadowmap : register(t0);

//matrix lightviewproj;
//float3 lightpos;

PSINPUT mainVS(VSINPUT In)
{
	PSINPUT O;

	O.position = mul(In.position, g_mWorldViewProjection);
	O.normal = mul(In.normal, (float3x3)g_mWorld);
	O.texcoord = In.texcoord;

	//O.texshadow = mul(In.position, lightviewproj);

	return O;
}

float4 mainPS(PSINPUT In) : SV_TARGET
{
	float tmp = shadowmap.Sample(pointsampler, In.texcoord).r;

	if (tmp == 0.f)
	{
		return float4(100, 100, 100, 1);
	}
	else
		return float4(100, 0, 0, 1);

	/*
	float4 debugcolor = float4(0, 100, 0, 1);

	float4 ambient = float4(100, 0, 0, 1);

	float4 diffuse = float4(100, 100, 100, 1);

	//re-homogenize position

	In.texshadow.xyz /= In.texshadow.w;

	// Don't illuminate something that the light does not touch

	if (1.f < abs(In.texshadow.x) || 1.f < abs(In.texshadow.y) || 1.f < abs(In.texshadow.z))
		return ambient;

	//move the texture coords from (-1,1) to (0,1)

	In.texshadow.x = (In.texshadow.x + 1.0) / 2.0;
	In.texshadow.y = (-In.texshadow.y + 1.0) / 2.0;

	// find the position in the shadow map of the current pixel

	float shadowMapDepth = shadowmap.Sample(pointsampler, In.texshadow.xy).r;

	// if the pixel lies behind the area in the shadow map, it is in the shadow

	if (shadowMapDepth < In.texshadow.z - shadowbias)
		return ambient;

	// calculate the illumination at this point

	float3 light = normalize(lightpos - In.normal.xyz);
		float dotlight = dot(normalize(In.normal), light);

	return ambient + dotlight * diffuse;*/
}