#define EPSILON 0.000001

cbuffer RayTraceDataCB : register(b2)
{
	float3 lightpos : packoffset(c0);
	float numindices : packoffset(c0.a);
};

struct Triangle
{
	float3 v1, v2, v3;
};

struct Ray
{
	float3 origin, direction;
};

Texture1D<float3> trimesh : register(t1); // stores all mesh considered for ray tracing
Texture1D<uint> indices : register(t2); // stores all indicies for ray tracing

//Buffer<float4> g_Buffer;

bool rayTriangleIntersection(const Triangle tri, const Ray ray)
{
	// edges of the triangle
	float3 edge1 = tri.v2 - tri.v1;
	float3 edge2 = tri.v3 - tri.v1;

	float3 p = cross(ray.direction, edge2); // calc the u parameter

	float det = dot(edge1, p);

	// no culling
	
	if (-EPSILON < det && det < EPSILON)
		return false;

	float inv_det = 1.f / det; // calc the inverse determinant

	float3 T = ray.origin - tri.v1; // calc distance from v1 to origin of ray

	float u = dot(T, p) * inv_det; // calc u param and test bound
	
	if (u < .0f || 1.f < u) // intersection outside of the triangle
		return false;

	float3 q = cross(T, edge1);

	float v = dot(ray.direction, q) * inv_det; // calculate v parameter and test bound
	
	if (v < .0f || u + v > 1.f) //  intersection is outside of the triangle
		return false;

	float t = dot(edge2, q) * inv_det;
	
	if (t > EPSILON) // ray intersects the triangle
	{


		return true;
	}
	
	// does not hit
	return false;
}

float4 rayTrace(float4 position, float4 diffuse, float4 ambient, float3 normal, matrix worldviewprojectionp)
{
	// launch shadow ray from primary ray

	Ray ray;

	ray.origin = (float3)(position);

	ray.direction = (float3)(mul(float4(lightpos, 1), worldviewprojectionp) - position);

	// check for intersections

	bool intersection = false;

	Triangle tri;

	for (int i = 0; i < numindices && !intersection; i+=3)
	{
		tri.v1 = (float3)mul(float4(trimesh[indices[i]], 1), worldviewprojectionp);
		tri.v2 = (float3)mul(float4(trimesh[indices[i + 1]], 1), worldviewprojectionp);
		tri.v3 = (float3)mul(float4(trimesh[indices[i + 2]], 1), worldviewprojectionp);
		
		intersection = rayTriangleIntersection(tri, ray);
	}

	if (!intersection)
		return diffuse;

	return float4(1, 0, 0, 1);
}