//#include "RayTraceH.hlsl"

#define EPSILON .00001

static const float PI = 3.141592;

cbuffer RayTraceDataCB : register(b2)
{
	float3 lightpos : packoffset(c0);
	float lightradius : packoffset(c0.a);
	float3 campos : packoffset(c1);
	float numshadowrays : packoffset(c1.a);
	float numindices : packoffset(c2);
};

struct Triangle
{
	float3 v1, v2, v3;
};

struct Ray
{
	float3 origin, direction;
};

struct Vertex
{
	float3 position;
};

struct Normal
{
	float3 normal;
};

struct RayTriangleReturn
{
	bool collision;
	float distance;
	uint index;
};

struct TexCoord
{
	float2 uv;
};

Texture1D<Vertex> trimesh : register(t1); // stores all mesh considered for ray tracing
Texture1D<Normal> trimeshn : register(t2); // stores all mesh normals considered for ray tracing
Texture1D<TexCoord> trimeshuv : register(t3); // stores all mesh texcoords considred for ray tracing
Texture1D<int> indices : register(t4); // stores all indicies for ray tracing

//Buffer<float4> g_Buffer;

RayTriangleReturn rayTriangleIntersection(const Triangle tri, const Ray ray, const int index)
{
	// no collision

	RayTriangleReturn rtr;

	rtr.collision = false;

	rtr.distance = -1;

	rtr.index = -1;

	// edges of the triangle
	float3 edge1 = tri.v2 - tri.v1;
	float3 edge2 = tri.v3 - tri.v1;

	float3 p = cross(ray.direction, edge2); // calc the u parameter

	float det = dot(edge1, p);

	// no culling
	
	if (abs(det) < EPSILON)
		return rtr;

	float inv_det = 1.f / det; // calc the inverse determinant

	float3 T = ray.origin - tri.v1; // calc distance from v1 to origin of ray

	float u = dot(T, p) * inv_det; // calc u param and test bound
	
	if (u < .0f || 1.f < u) // intersection outside of the triangle
		return rtr;

	float3 q = cross(T, edge1);

	float v = dot(ray.direction, q) * inv_det; // calculate v parameter and test bound
	
	if (v < .0f || u + v > 1.f) //  intersection is outside of the triangle
		return rtr;

	float t = dot(edge2, q) * inv_det;
	
	if (t > EPSILON) // ray intersects the triangle
	{
		rtr.collision = true;

		rtr.distance = t;

		rtr.index = index;

		return rtr;
	}
	
	// does not hit
	return rtr;
}

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

	return rtr;
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

float4 reflectionRefractionCalculation(Ray ray, float3 hitpos, float3 normal, matrix world, int lightval, float4 color)
{
	// TODO: calc lightval

	RayTriangleReturn rtr;
	
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
				ray.direction * rtr.distance,
				trimeshuv[indices[rtr.index]].uv,
				trimeshuv[indices[rtr.index + 1]].uv,
				trimeshuv[indices[rtr.index + 2]].uv
				);

			diffusemap.Sample(linearsampler, uv);
		}
	} while (0 < lightval && rtr.collision);

	return color / lightval;
}

float4 rayTrace(float4 position, float3 normal, float4 ambient, float4 diffuse, float4 specular, matrix world)
{
	// run a standard phong shader

	// launch shadow ray from primary ray

	// launch reflection ray from primary ray
	
	Ray ray;

	ray.origin = campos;

	ray.direction = (position - ray.origin);
	
	return reflectionRefractionCalculation(ray, position, normal, world, 3, float4(1, 1, 1, 1)) * shadowCalculation(position, world);
	
	//return diffuse * shadowCalculation(position, world);
}

// NOT USED

// calculate blinn phong color values

float4 runBlinnPhong(float3 normal, float ambient, float diffuse, float specular)
{
	float4 Ia = ambient;
	float4 Id = diffuse * saturate(dot(normal, float4(lightpos, 1)));
	float4 Is = float4(1, 1, 1, 1);// specular * pow(saturate(dot(normal, normalize(-+V)), M.A);

	return Ia + (Id + Is) * float4(1, 1, 1, 1);
}