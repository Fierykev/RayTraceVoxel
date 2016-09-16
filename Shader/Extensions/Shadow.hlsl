cbuffer ShadowDataCB : register(b0)
{
	matrix lightviewproj : packoffset(c0);
	float3 lightpos : packoffset(c4);
	float shadowbias : packoffset(c4.a);
};

cbuffer ShadowMapCB : register(b1)
{
	int width;
	int height;
};

Texture2D shadowmap : register(t0);

float4 ShadowCalc(float4 position, matrix world)
{
	float4 texshadow = mul(position, lightviewproj);

	return texshadow;
}

float4 ShadowColorCalc(float4 diffuse, float4 ambient, float3 normal, float4 texshadow)
{
	//re-homogenize position

	texshadow.xyz /= texshadow.w;

	// Don't illuminate something that the light does not touch

	if (1.f < abs(texshadow.x) || 1.f < abs(texshadow.y) || 1.f < abs(texshadow.z))
		return ambient;

	//move the texture coords from (-1,1) to (0,1)

	texshadow.x = (texshadow.x + 1.0) / 2.0;
	texshadow.y = (-texshadow.y + 1.0) / 2.0;

	// find the position in the shadow map of the current pixel

	float shadowMapDepth = shadowmap.Sample(pointsampler, texshadow.xy).r;

	// if the pixel lies behind the area in the shadow map, it is in the shadow
	/*
	if (shadowMapDepth < texshadow.z - shadowbias)
	{
	float shadowcomp = 0;// (texshadow.z - shadowMapDepth) / shadowMapDepth;

	return shadowcomp * diffuse + ambient;
	}*/

	texshadow.z -= shadowbias;

	float sum = 0;
	
	const int searcharea = 4; // 4 x 4

	//if (shadowMapDepth < texshadow.z - shadowbias) // only check shaded areas to save time

	for (int y = -searcharea / 2; y < searcharea / 2; y++)
	{
		for (int x = -searcharea / 2; x < searcharea / 2; x++)
		{
			//if ()
			sum += shadowmap.SampleCmpLevelZero(comparesamples, float2(texshadow.x + x * 1.f / width, texshadow.y + y  * 1.f / height), texshadow.z);
		}
	}

	sum /= pow(searcharea, 2);

	// correct for odd lines where there should not be shadow

	//if (texshadow.z - shadowbias <= shadowMapDepth && sum < 2)
		//return float4(1, 0, 0, 1);

	//else
		//sum = 1;
	/*
	if (sum != 0 && shadowMapDepth < texshadow.z - shadowbias)
	{
		if ( sum < 1)
			return float4(1, 0, 0, 1);
	}
	*/
	
	//float shadowcomp = (texshadow.z - shadowMapDepth);
	//if (shadowcomp < 1)
		//return float4(1, 0, 0, 0);
	// calculate the illumination at this point

	float3 light = normalize(lightpos - normal.xyz);
	float dotlight = dot(normalize(normal), light);

	return ambient + sum * dotlight * diffuse;// *shadowcomp;
}