//--------------------------------------------------------------------------------------
// File: BasicHLSL11_VS.hlsl
//
// The vertex shader file for the BasicHLSL11 sample.  
// 
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Globals
//--------------------------------------------------------------------------------------
cbuffer cbPerObject : register(b0)
{
	matrix		g_mWorldViewProjection	: packoffset(c0);
	matrix		g_mWorld				: packoffset(c4);
	matrix		g_mView					: packoffset(c8);
	float4		g_vEye					: packoffset(c12); // Camera's location
	float3		g_LightDir				: packoffset(c13); // Light's direction in world space
};

//--------------------------------------------------------------------------------------
// Input / Output structures
//--------------------------------------------------------------------------------------
struct VS_INPUT
{
	float3 inPositionOS  : POSITION;
	float3 vInNormalOS : NORMAL;
	float2 inTexCoord : TEXCOORD0;
	float3 vInTangentOS : TANGENT;
	float3 vInBinormalOS : BINORMAL;
	/*
	float4 inPositionOS	: POSITION;
	float3 vInNormalOS		: NORMAL;
	float2 inTexCoord	: TEXCOORD0;*/
};

struct VS_OUTPUT
{
	float3 position          : POSITION;
	float2 texCoord          : TEXCOORD0;
	float3 vLightTS          : TEXCOORD1;   // light vector in tangent space, denormalized
	float3 vViewTS           : TEXCOORD2;   // view vector in tangent space, denormalized
	float2 vParallaxOffsetTS : TEXCOORD3;   // Parallax offset vector in tangent space
	float3 vNormalWS         : TEXCOORD4;   // Normal vector in world space
	float3 vViewWS           : TEXCOORD5;   // View vector in world space
};

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT main(VS_INPUT Input)
{
	float    g_fBaseTextureRepeat = 1.0f;      // The tiling factor for base and normal map textures
	float    g_fHeightMapScale = .1;         // Describes the useful range of values for the height field
	
	VS_OUTPUT Out;
	/*
	VS_OUTPUT Output;

	Output.position = mul(Input.inPositionOS, g_mWorldViewProjection);
	Output.vNormalWS = mul(Input.vInNormalOS, (float3x3)g_mWorld);
	Output.texCoord = Input.inTexCoord;

	return Output;
	*/
	// Transform and output input position 
	Out.position = mul(Input.inPositionOS, g_mWorldViewProjection);

	// Propagate texture coordinate through:
	
	Out.texCoord = Input.inTexCoord * g_fBaseTextureRepeat;
	/*
	// Transform the normal, tangent and binormal vectors from object space to homogeneous projection space:
	float3 vNormalWS = mul(Input.vInNormalOS, (float3x3) g_mView);

	float3 vTangentWS = mul(Input.vInTangentOS, (float3x3) g_mView);

	float3 vBinormalWS = mul(Input.vInBinormalOS, (float3x3) g_mView);

		// Propagate the world space vertex normal through:   
	Out.vNormalWS = vNormalWS;

	vNormalWS = normalize(vNormalWS);
	vTangentWS = normalize(vTangentWS);
	vBinormalWS = normalize(vBinormalWS);


	// Compute position in world space:
	float4 vPositionWS = mul(Input.inPositionOS, g_mView);

		// Compute and output the world view vector (unnormalized):
		float3 vViewWS = g_vEye - vPositionWS;
		Out.vViewWS = vViewWS;

	// Compute denormalized light vector in world space:
	float3 vLightWS = g_LightDir;

		// Normalize the light and view vectors and transform it to the tangent space:
		float3x3 mWorldToTangent = float3x3(vTangentWS, vBinormalWS, vNormalWS);

		// Propagate the view and the light vectors (in tangent space):
	Out.vLightTS = mul(vLightWS, mWorldToTangent);
	Out.vViewTS = mul(mWorldToTangent, vViewWS);

	// Compute the ray direction for intersecting the height field profile with 
	// current view ray. See the above paper for derivation of this computation.

	// Compute initial parallax displacement direction:
	float2 vParallaxDirection = normalize(Out.vViewTS.xy);

		// The length of this vector determines the furthest amount of displacement:
		float fLength = length(Out.vViewTS);
	float fParallaxLength = sqrt(fLength * fLength - Out.vViewTS.z * Out.vViewTS.z) / Out.vViewTS.z;

	// Compute the actual reverse parallax displacement vector:
	
	Out.vParallaxOffsetTS = ComputeTangent(Input);// *fParallaxLength;//float2(g_mWorld[0][0], 1);// *fParallaxLength;
	
	// Need to scale the amount of displacement to account for different height ranges
	// in height maps. This is controlled by an artist-editable parameter:
	//Out.vParallaxOffsetTS *= g_fHeightMapScale;
	*/
	return Out;
}

