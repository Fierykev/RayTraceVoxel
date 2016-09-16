#define EPSILON .00001

struct RayTriangleReturn
{
	bool collision;
	float distance;
	uint index;
};

struct Triangle
{
	float3 v1, v2, v3;
};

struct Ray
{
	float3 origin, direction;
};

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