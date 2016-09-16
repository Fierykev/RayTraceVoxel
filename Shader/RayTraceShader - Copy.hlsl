#include "Extensions/Sampler.hlsl"
#include "Extensions/LinetoSquare.hlsl"

static const float PI = 3.141592;

cbuffer StandardShaderCB : register(b0)
{
	matrix worldviewprojection : packoffset(c0);
	matrix world : packoffset(c4);
}

cbuffer MaterialCB : register(b1)
{
	float4 ambient : packoffset(c0);
	float4 diffuse : packoffset(c1);
	bool texturei : packoffset(c2);
}

cbuffer RayTraceDataCB : register(b2)
{
	float3 lightpos : packoffset(c0);
	float lightradius : packoffset(c0.a);
	float3 campos : packoffset(c1);
	float numshadowrays : packoffset(c1.a);
	float numindices : packoffset(c2);
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
	float4 worldpos : TEXCOORD1;
};

struct Vertex
{
	float3 position;
};

struct Normal
{
	float3 normal;
};

struct TexCoord
{
	float2 uv;
};

Texture2D diffusemap : register(t0);
Texture1D<Vertex> trimesh : register(t1); // stores all mesh considered for ray tracing
Texture1D<Normal> trimeshn : register(t2); // stores all mesh normals considered for ray tracing
Texture1D<TexCoord> trimeshuv : register(t3); // stores all mesh texcoords considred for ray tracing
Texture1D<int> indices : register(t4); // stores all indicies for ray tracing

// the light position offset by a certain value

float3 lighPositionOffset(float3 offset)
{
	return lightpos + offset;
}

// checks for collisions between the ray and the triangle mesh

bool rayMeshIntersection(Ray ray, matrix world)
{
	Triangle tri;

	RayTriangleReturn rtr;

	rtr.collision = false;

	for (int i = 0; i < numindices && !rtr.collision; i += 3)
	{
		tri.v1 = (float3)mul(float4(trimesh[indices[i]].position, 1), world);
		tri.v2 = (float3)mul(float4(trimesh[indices[i + 1]].position, 1), world);
		tri.v3 = (float3)mul(float4(trimesh[indices[i + 2]].position, 1), world);

		rtr = rayTriangleIntersection(tri, ray, i);
	}

	return rtr.collision;
}

// checks for the closest collisions between the ray and the triangle mesh

RayTriangleReturn rayMeshClosestIntersection(Ray ray, matrix world)
{
	linetoSquare(ray.origin, ray, size);

	/*
	Triangle tri;

	RayTriangleReturn rtr;

	RayTriangleReturn rtrtmp;

	rtr.collision = false;

	for (int i = 0; i < numindices; i += 3)
	{
		tri.v1 = (float3)mul(float4(trimesh[indices[i]].position, 1), world);
		tri.v2 = (float3)mul(float4(trimesh[indices[i + 1]].position, 1), world);
		tri.v3 = (float3)mul(float4(trimesh[indices[i + 2]].position, 1), world);

		rtrtmp = rayTriangleIntersection(tri, ray, i);

		if (rtrtmp.collision)
			if (!rtr.collision || rtrtmp.distance < rtr.distance)
				rtr = rtrtmp;
	}

	return rtr;*/
}

// calculate if the object is in the shadow and its brightness

float shadowCalculation(float4 position, matrix world)
{
	// iterate over the area light (normal polygon of point numshadowrays)

	Ray ray;

	ray.origin = (float3)(position);

	float anglebetweenpoints = 2.f * PI / numshadowrays;

	float3 ptlighpos;

	float lightintensityrays = 0; // number of light points that make it

	RayTriangleReturn rtr;

	float maxdistance = 5.f;

	for (float i = 0; i < numshadowrays; i++)
	{
		// calculate point position

		ptlighpos = lighPositionOffset(float3(lightradius * cos(anglebetweenpoints * i), 0, lightradius * sin(anglebetweenpoints * i)));

		ray.direction = (float3)(mul(ptlighpos, world) - position);

		// check for intersection

		if (rayMeshIntersection(ray, world))
			lightintensityrays++;
	}

	return 1.f - lightintensityrays / numshadowrays;
}

// reflection

// based on http://stackoverflow.com/questions/13689632/converting-vertex-normals-to-face-normals

float3 vertexNormaltoFaceNormal(float3 v0, float3 v1, float3 v2, float3 n0, float3 n1, float3 n2)
{
	float3 p0 = v1 - v0;
	float3 p1 = v2 - v0;
	float3 facenormal = cross(p0, p1);

	float3 vertexnormal = (n0 + n1 + n2) / 3.f; // average the normals
	float direction = dot(facenormal, vertexnormal); // calculate the direction

	return direction < .0f ? -facenormal : facenormal;
}

// get the distance between vectors

float distance(float3 v0, float3 v1)
{
	return sqrt(
		(v0.x - v1.x) * (v0.x - v1.x) +
		(v0.y - v1.y) * (v0.y - v1.y) +
		(v0.z - v1.z) * (v0.z - v1.z)
		);
}

// get the vector magnitude

float magnitude(float3 v)
{
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

// get the texture coordinate within the triangle

// based on http://answers.unity3d.com/questions/383804/calculate-uv-coordinates-of-3d-point-on-plane-of-m.html

float2 getTexCoord(float3 v0, float3 v1, float3 v2, float3 pt, float2 uv0, float2 uv1, float2 uv2)
{
	// get the vector from the point to the triangle verts
	float3 vector0 = v0 - pt;
	float3 vector1 = v1 - pt;
	float3 vector2 = v2 - pt;

		// calculate areas as well as factors

	float a0 = magnitude(cross(v0 - v1, v0 - v2)); // main triangle area
	float a1 = magnitude(cross(vector1, vector2)) / a0; // v0 area / a0
	float a2 = magnitude(cross(vector2, vector0)) / a0; // v1 area / a0
	float a3 = magnitude(cross(vector0, vector1)) / a0; // v2 area / a0

	// find the uv coord

	return uv0 * a1 + uv1 * a2 + uv2 * a3;
}


// calculate the object's reflection
// based on https://www.cs.unc.edu/~rademach/xroads-RT/RTarticle.html

float4 reflectionRefractionCalculation(Ray ray, float3 hitpos, float3 normal, matrix world, int lightval, float4 diffuse, float4 ambient, float shadow)
{
	// TODO: calc lightval

	RayTriangleReturn rtr;

	float4 color = float4(1,1,1,1);

	if (shadow > 0.f) // don't compute if no light hits the area
	{
		do
		{
			lightval--;

			// calculate reflected ray

			ray.origin = hitpos;

			ray.direction = normalize(ray.direction + (-dot(float4(normal, 1), float4(ray.direction, 1)) * normal * 2.f));

			// check for hit

			rtr = rayMeshClosestIntersection(ray, world);

			// setup to run again

			if (rtr.collision)
			{
				// convert vertex normal to face normal

				normal = vertexNormaltoFaceNormal(
					trimesh[indices[rtr.index]].position,
					trimesh[indices[rtr.index + 1]].position,
					trimesh[indices[rtr.index + 2]].position,
					trimeshn[indices[rtr.index]].normal,
					trimeshn[indices[rtr.index + 1]].normal,
					trimeshn[indices[rtr.index + 2]].normal
					);

				const float2 uv = getTexCoord(
					trimesh[indices[rtr.index]].position,
					trimesh[indices[rtr.index + 1]].position,
					trimesh[indices[rtr.index + 2]].position,
					ray.origin + ray.direction * rtr.distance,
					trimeshuv[indices[rtr.index]].uv,
					trimeshuv[indices[rtr.index + 1]].uv,
					trimeshuv[indices[rtr.index + 2]].uv
					);

				color *= diffusemap.Sample(linearsampler, uv);
			}
		} while (0 < lightval && rtr.collision);
	}

	return diffuse * (ambient + color *shadow);// / lightval;
}

float4 rayTrace(float4 position, float3 normal, float4 ambient, float4 diffuse, float4 specular, matrix world)
{
	// run a standard phong shader

	// launch shadow ray from primary ray

	// launch reflection ray from primary ray

	Ray ray;

	ray.origin = campos;

	ray.direction = (position - ray.origin);
	//* shadowCalculation(position, world)
	return shadowCalculation(position, world);// reflectionRefractionCalculation(ray, position, normal, world, 3, diffuse, specular, shadowCalculation(position, world));

	//if ((int)trimeshuv[indices[0]].uv.x == 1)
		//return float4(0, 0, 1, 1);
	//trimeshuv[indices[rtr.index]].
	//return diffusemap.Sample(linearsampler, uv);

	//return diffuse * shadowCalculation(position, world);
}

PSINPUT mainVS(VSINPUT In)
{
	PSINPUT O;

	O.position = mul(In.position, worldviewprojection);
	O.normal = mul(In.normal, (float3x3)world);
	O.texcoord = In.texcoord;
	O.worldpos = mul(In.position, world);

	return O;
}

float4 mainPS(PSINPUT In) : SV_TARGET
{
	float4 diffuse_color = diffuse;

	if (texturei)
		diffuse_color *= diffusemap.Sample(linearsampler, In.texcoord);
	
	//float scale = 100.f;
	//if (In.worldpos.y < 0)
		//return float4(1, 0, 0, 1);
	//return float4(In.worldpos.z / scale, In.worldpos.z / scale, In.worldpos.z / scale, 1);
	return rayTrace(In.worldpos, In.normal, ambient, diffuse_color, float4(1, 1, 1, 1), world);
}