SamplerState linearsampler : register(s0);

#define EPSILON .00001

#define DISTANCERROR .00001

#define MAXTRIANGLES 100

#define swap(a, b) {float t; t = a; a = b; b= t;}

static const float PI = 3.141592;

cbuffer StandardShaderCB : register(b0)
{
	matrix worldviewprojection : packoffset(c0);
	matrix world : packoffset(c4);
};

cbuffer MaterialCB : register(b1)
{
	float4 ambient : packoffset(c0);
	float4 diffuse : packoffset(c1);
	bool texturei : packoffset(c2);
};

cbuffer RayTraceDataCB : register(b2)
{
	float3 lightpos : packoffset(c0);
	float lightradius : packoffset(c0.a);
	float3 campos : packoffset(c1);
	float numshadowrays : packoffset(c1.a);
	float numindices : packoffset(c2);
};

cbuffer SquareGridCB : register(b3)
{
	float3 lwh : packoffset(c0);
	float size : packoffset(c0.a);
	float3 start : packoffset(c1);
	float maxnum : packoffset(c1.a);
	float numperlink : packoffset(c2);
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

struct Triangle
{
	float3 v1, v2, v3;
};

struct Ray
{
	float3 origin, direction;
};

Texture2D diffusemap : register(t0);
Texture1D<Vertex> trimesh : register(t1); // stores all mesh considered for ray tracing
Texture1D<Normal> trimeshn : register(t2); // stores all mesh normals considered for ray tracing
Texture1D<TexCoord> trimeshuv : register(t3); // stores all mesh texcoords considred for ray tracing
Texture1D<unsigned int> indices : register(t4); // stores all indicies for ray tracing
Texture1D<unsigned int> gridmap : register(t5); // stores the grid map
Texture2D<unsigned int> gridmap2D : register(t6); // stores the grid map2D

/**
RayTriangle Collision
**/

void rayTriangleIntersection(const Triangle tri, const Ray ray, out bool collision, out float distance)
{
	// no collision

	collision = false;

	distance = -1;

	// edges of the triangle
	float3 edge1 = tri.v2 - tri.v1;
	float3 edge2 = tri.v3 - tri.v1;

	float3 p = cross(ray.direction, edge2); // calc the u parameter

	float det = dot(edge1, p);

	// no culling

	if (abs(det) < EPSILON)
		return;

	float inv_det = 1.f / det; // calc the inverse determinant

	float3 T = ray.origin - tri.v1; // calc distance from v1 to origin of ray

	float u = dot(T, p) * inv_det; // calc u param and test bound

	if (u < .0f || 1.f < u) // intersection outside of the triangle
		return;

	float3 q = cross(T, edge1);

	float v = dot(ray.direction, q) * inv_det; // calculate v parameter and test bound

	if (v < .0f || u + v > 1.f) //  intersection is outside of the triangle
		return;

	float t = dot(edge2, q) * inv_det;

	if (t > EPSILON) // ray intersects the triangle
	{
		collision = true;

		distance = t;
	}

	// does not hit
}

/**
Grid Map
**/

/*
get a value at the i location in the grid as if it were not 2 dimensions
*/

unsigned int atGrid(unsigned int i)
{
	unsigned int width, height;

	gridmap2D.GetDimensions(width, height);

	uint2 index;

	index.y = i / width;

	index.x = i - index.y * width;

	return index.y < height ? gridmap2D[index] : 0;
}

/*
Get the loctaion of a box in the map.
*/

unsigned int getBoxinMap(unsigned int boxnum)
{
	if (maxnum <= boxnum || numperlink == 0)
		return 0;
	
	const unsigned int linklistsize = maxnum / numperlink;

	unsigned int pos = boxnum / numperlink; // find the start of the link list using the key

	while (pos < linklistsize && atGrid(pos) != 0 || pos >= linklistsize && atGrid(pos) != 0 && atGrid(pos + 2) != boxnum) // loop till the while reaches the end of the link list or finds the box
	{
		// move to the next position
		
		pos = atGrid(pos);
	}
	
	if (pos < linklistsize && atGrid(pos) == 0 || pos >= linklistsize && atGrid(pos) == 0 && atGrid(pos + 2) != boxnum) // box is not found
		return 0;
	
	return pos + 1;
}

/*
Get the next loctaion of a triangle in the map.
*/

unsigned int getNextTriangle(unsigned int pos)
{
	return atGrid(pos) != 0 ? atGrid(pos) : 0;
}

/*
Get the triangle number by moving past the header
*/

unsigned int getTriangleData(unsigned int pos)
{
	return pos + 1;
}

/**
Convert the line to a square.
**/

void finalGraph(float3 linexyz[2], float x0, float x1, float y0, float y1, float z0, float z1, inout float4 color)
{
	const float xmin = min(x0, x1);

	const float ymin = min(y0, y1);

	const float zmin = min(z0, z1);

	// check if the variables are non-negative numbers

	if (xmin < 0 || ymin < 0 || zmin < 0 ||
		lwh.x <= xmin || lwh.y <= ymin || lwh.z <= zmin)
		return;

	// check the spot in the grid

	unsigned int pos = getBoxinMap(floor(xmin / size) + floor(zmin / size) * lwh.x + floor(ymin / size) * lwh.x * lwh.y);

	// calculate the ray
	
	Ray ray;

	ray.origin = linexyz[0];

	ray.direction = linexyz[1] - linexyz[0];

	if (pos != 0)
		color = float4((float)(pos % 10) / 10.f, 0, 0, 1);
	
	/*
	Triangle tri;

	bool collision;

	float distance;
	
	while ((pos = getNextTriangle(pos)) != 0) // loop till no more tests
	{
		const unsigned int index = getTriangleData(pos);

		tri.v1 = trimesh[index].position;

		tri.v2 = trimesh[index + 1].position;

		tri.v3 = trimesh[index + 2].position;

		rayTriangleIntersection(tri, ray, collision, distance);

		if (collision && DISTANCERROR < distance)
		{
			color = float4(1, 0, 0, 1);

			return;
		}
	}*/
}

void yzGraph(float3 linexyz[2], float x0, float x1, float y0, float y1, float m2, float zinc, inout float4 color)
{
	const float z0 = isfinite(m2) ? linexyz[0].z + m2 * (y0 - linexyz[0].y) : linexyz[0].z;

	const float z1 = isfinite(m2) ? linexyz[0].z + m2 * (y1 - linexyz[0].y) : linexyz[1].z;

	int distance;

	float z;

	if (zinc == 1)
	{
		z = floor((z0 + size) / size) * size;

		distance = ceil((z1 - size) / size) * size - z;
	}
	else
	{
		z = ceil((z0 - size) / size) * size;

		distance = z - floor((z1 + size) / size) * size;
	}

	if (distance != -1)
	{
		finalGraph(linexyz, x0, x1, y0, y1, z0, z, color);
		
		/*[unroll(distance)]
		for (; 0 < distance; distance--)
		{
			finalGraph(linexyz, x0, x1, y0, y1, z, z + zinc * size, color);

			z += zinc * size;
		}*/
		
		if (floor(z0) != floor(z1)) // not in the same square
			finalGraph(linexyz, x0, x1, y0, y1, z, z1, color);
	}
	else
		finalGraph(linexyz, x0, x1, y0, y1, z0, z1, color);
}

void xyzGraph(float3 linexyz[2], float x0, float x1, float m, float m2, float yinc, float zinc, inout float4 color)
{
	const float y0 = isfinite(m) ? linexyz[0].y + m * (x0 - linexyz[0].x) : linexyz[0].y;

	const float y1 = isfinite(m) ? linexyz[0].y + m * (x1 - linexyz[0].x) : linexyz[1].y;

	int distance;

	float y;

	if (yinc == 1)
	{
		y = floor((y0 + size) / size) * size;

		distance = ceil((y1 - size) / size) * size - y;
	}
	else
	{
		y = ceil((y0 - size) / size) * size;

		distance = y - floor((y1 + size) / size) * size;
	}

	if (distance != -1)
	{
		yzGraph(linexyz, x0, x1, y0, y, m2, zinc, color);
		/*
		[unroll(distance)]
		for (; 0 < distance; distance--)
		{
			yzGraph(linexyz, x0, x1, y, y + yinc * size, m2, zinc, color);

			y += yinc * size;
		}*/
		
		if (floor(y0) != floor(y1)) // not in the same square
			yzGraph(linexyz, x0, x1, y, y1, m2, zinc, color);
	}
	else
		yzGraph(linexyz, x0, x1, y0, y1, m2, zinc, color);
}

float4 linetoSquare(float3 linep[2], float4 color)
{
	// move the linexyz into the grid

	float3 linexyz[2];

	linexyz[0] = linep[0] - start;

	linexyz[1] = linep[1] - start;

	// create array to store triangles that have been looked at

	//unsigned int trilist[MAXTRIANGLES];

	// check if the starting point is within the grid TODO: CHECK STARTING POINT CORRECTLY

	// draw the x, y view of the linexyz

	float m = (linexyz[1].y - linexyz[0].y) / (linexyz[1].x - linexyz[0].x); // calculate the slope of the linexyz xy

	float m2 = (linexyz[1].z - linexyz[0].z) / (linexyz[1].y - linexyz[0].y); // calculate the slope of the linexyz yz

	float tmpy[2];

	const int xinc = linexyz[0].x < linexyz[1].x ? 1 : -1; // x increment

	const int yinc = linexyz[0].y < linexyz[1].y ? 1 : -1; // y increment

	const int zinc = linexyz[0].z < linexyz[1].z ? 1 : -1; // z increment

	// deal with verticle case

	if (!isfinite(m))
	{
		xyzGraph(linexyz, linexyz[0].x, linexyz[1].x, m, m2, yinc, zinc, color);
	}
	else
	{
		float x;

		int distance;

		if (xinc == 1)
		{
			x = floor((linexyz[0].x + size) / size) * size;

			distance = ceil((linexyz[1].x - size) / size) * size - x;
		}
		else
		{
			x = ceil((linexyz[0].x - size) / size) * size;

			distance = x - floor((linexyz[1].x + size) / size) * size;
		}

		if (distance != -1)
		{
			xyzGraph(linexyz, linexyz[0].x, x, m, m2, yinc, zinc, color);
			
			for (; 0 < distance; distance--)
			{
				xyzGraph(linexyz, x, x + xinc * size, m, m2, yinc, zinc, color);

				x += xinc * size;
			}
			
			xyzGraph(linexyz, x, linexyz[1].x, m, m2, yinc, zinc, color);
		}
		else
			xyzGraph(linexyz, linexyz[0].x, linexyz[1].x, m, m2, yinc, zinc, color);
	}

	return color;
}

float4 rayTrace(float4 position, float3 normal, float4 ambient, float4 diffuse, float4 specular, matrix world)
{
	// run a standard phong shader

	// launch shadow ray from primary ray

	// launch reflection ray from primary ray

	float3 tmp[2];

	tmp[0] = position;

	tmp[1] = lightpos;
	/*
	float4 color = float4(0, 0, 0, 0);

	finalGraph(tmp, 0, 0, 0, 0, 0, 0, color);

	return color;*/
	//getBoxinMap(32335) != 0)
	
	return linetoSquare(tmp, float4(0, 0, 0, 0));
}

float4 mainPS(PSINPUT In) : SV_TARGET
{
	float4 diffuse_color = diffuse;

	if (texturei)
		diffuse_color *= diffusemap.Sample(linearsampler, In.texcoord);

	return rayTrace(In.worldpos, In.normal, ambient, diffuse_color, float4(1, 1, 1, 1), world);
}