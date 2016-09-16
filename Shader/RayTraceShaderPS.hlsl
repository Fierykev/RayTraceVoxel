SamplerState linearsampler : register(s0);

#define M_PI 3.14159

#define M 2147483647
#define A 16807
#define Q ( M / A )
#define R ( M % A )

#define RAYDISTANCE 100.f

#define EPSILON .00001

#define FLT_MAX 3.402823466e+38F

#define DISTANCERE .00001

#define LPERROR .00001 // error for line plane collision

#define MAXTRIANGLEMAP 500

#define NUMPERKEY 10000

#define swap(a, b) {float t; t = a; a = b; b= t;}

#define magnitude(v) sqrt(v.x * v.x + v.y * v.y + v.z * v.z)

#define sum(a) (a.x + a.y + a.z)

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
	float4 specular : packoffset(c2);
	float shininess : packoffset(c3);
	bool texturei : packoffset(c3.y);
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
// make static
Texture2D diffusemap : register(t0);
Texture1D<Vertex> trimesh : register(t1); // stores all mesh considered for ray tracing
Texture1D<Normal> trimeshn : register(t2); // stores all mesh normals considered for ray tracing
Texture1D<TexCoord> trimeshuv : register(t3); // stores all mesh texcoords considred for ray tracing
Texture1D<unsigned int> indices : register(t4); // stores all indicies for ray tracing
Texture2D<unsigned int> gridmap2D : register(t5); // stores the grid map2D

Texture1D<float> random : register(t6);

// random variable index
groupshared int index = 0;

/**
Random
based on http://gamedev.stackexchange.com/questions/32681/random-number-hlsl

http://stackoverflow.com/questions/6275593/how-to-write-your-own-random-number-algorithm
**/
// Knuth Seminumerical algorithms the art of computer programming
// generate random on cpu and pass it over
// NVidia (DX10)
// graduate intro course for graphics

// bounding voume hierarchy
// kd tree (surface area heuristic)

// while while

// if while

// fresnel

// scene graph structure warren hunt build acceleration structure (conluded rebuild it based on scene graph)
float rand(inout float2 seed)
{/*
	index++;

	unsigned int width;

	random.GetDimensions(width);

	if (width <= index)
		index = 0;

	return random[index];
	*/
	seed = (frac(sin(dot(seed, float2(12.9898, 78.233)*2.0)) * 43758.5453));

	return abs(seed.x + seed.y) * 0.5;
}

unsigned int rand(inout float2 seed, float min, float max)
{
	return rand(seed) * (max - min) + min;
}

int randsign(inout float2 seed)
{
	return rand(seed, 0, 1) < .5 ? -1 : 1;
}

/*
Get the sign of a float.
*/

float getSign(float n)
{
	if (n == 0)
		return 0;

	return n < 0 ? -1.f : 1.f;
}

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
Normals
**/

// get the vertex normal

float3 vertexNormaltoFaceNormal(float3 v0, float3 v1, float3 v2, float3 n0, float3 n1, float3 n2)
{
	float3 p0 = v1 - v0;
	float3 p1 = v2 - v0;
	float3 facenormal = cross(p0, p1);

	float3 vertexnormal = (n0 + n1 + n2) / 3.f; // average the normals
	float direction = dot(facenormal, vertexnormal); // calculate the direction

	return direction < .0f ? -facenormal : facenormal;
}

/**
Texture Coord
**/

// get the texture coordinate within the triangle

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

/*
Collision between a line and a plane.
Based on https://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm.
*/

bool linePlaneCollision(out float t, float3 v0, float3 v1, float3 normal, float3 planept)
{
	// calculate d

	float d = -dot(planept, normal);

	float3 dir = v1 - v0;

	// calculate t
	
	t = -(dot(v0, normal) + d) / dot(dir, normal);

	if (!isfinite(t) || 1.f + LPERROR < t || t < 0.f - LPERROR)
		return false;

	return true;
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
	if ((unsigned int)maxnum <= boxnum || numperlink == 0)
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
	return atGrid(pos);
}

/*
Get the triangle number by moving past the header
*/

unsigned int getTriangleData(unsigned int pos)
{
	return atGrid(pos + 1);
}

bool linetoSquare(float3 linep[2], out unsigned int collisionindex, out float3 collisionpt)
{
	// TMP

	collisionindex = 0;

	collisionpt = float3(0, 0, 0);

	// move the line into the grid

	float3 linexyz[2];

	linexyz[0] = linep[0] - start;

	linexyz[1] = linep[1] - start;

	float linedistance;
	
	float minlinedistance = FLT_MAX;

	const unsigned int vtlinklistsize = ceil(maxnum / NUMPERKEY);

	if (MAXTRIANGLEMAP < vtlinklistsize) // no room
		return false;

	unsigned int vtpos = vtlinklistsize;

	//unsigned int viewedtri[MAXTRIANGLEMAP];

	// create the key link list
	/*
	for (int init = 0; init < vtlinklistsize; init++)
		viewedtri[init] = 0;
	*/
	unsigned int viewedtrisize = 0;
	
	for (unsigned int i = 0; i < 2; i++)
	{
		// check if the point is within the grid
		
		if (linexyz[i].x < 0 || linexyz[i].y < 0 || linexyz[i].z < 0 ||
			lwh.x <= linexyz[i].x || lwh.y <= linexyz[i].y || lwh.z <= linexyz[i].z)
		{
			// find where the point intersects the grid first

			[unroll(3)]
			for (unsigned int j = 0; j < 3; j++)
			{
				if(linePlaneCollision(linedistance, linexyz[i], linexyz[(i + 1) % 2], float3(j == 0, j == 1, j == 2), 0) &&
					linedistance < minlinedistance)
					minlinedistance = linedistance;
			}

			[unroll(3)]
			for (unsigned int k = 0; k < 3; k++)
			{
				if (linePlaneCollision(linedistance, linexyz[i], linexyz[(i + 1) % 2], float3(k == 0, k == 1, k == 2), lwh[k]) &&
					linedistance < minlinedistance)
					minlinedistance = linedistance;
			}

			// no intersection with the grid
			
			if (minlinedistance == FLT_MAX)
				return false;
			
			// get the position

			linexyz[i] = linexyz[i] + minlinedistance * (linexyz[(i + 1) % 2] - linexyz[i]);
		}
	}
	
	// get the half size

	const float halfsize = size / 2;

	// get the direction of the linexyz

	const float3 dir = float3(getSign(linexyz[1].x - linexyz[0].x), getSign(linexyz[1].y - linexyz[0].y), getSign(linexyz[1].z - linexyz[0].z));

	float distance;

	float mindistance;

	// get the starting position (the middle of the square is used)

	float3 pos = float3(floor(linexyz[0].x / size) * size + halfsize, floor(linexyz[0].y / size) * size + halfsize, floor(linexyz[0].z / size) * size + halfsize);

	float3 nextpos = pos;

	float3 tmppos;

	// calculate the ray

	Ray ray;

	ray.origin = linexyz[0] + start;

	ray.direction = linexyz[1] - linexyz[0];

	Triangle tri;

	bool collision, collisionfound = false;

	float raydistance, raypt;
	
	float collisiondistance = FLT_MAX;

	bool tricheck;

	// pass the starting position to be checked

	// loop until we reach the ending point

	bool posupdate = true; // make certain the position is updated or we have reached the end point

	while (posupdate)
	{
		// check the square on the grid

		// check if the variables are non-negative numbers

		const float xmin = pos.x - halfsize, ymin = pos.y - halfsize, zmin = pos.z - halfsize;
		
		if (0 <= xmin && xmin < lwh.x &&
			0 <= ymin && ymin < lwh.z &&
			0 <= zmin && zmin < lwh.x)
		{
			// check the spot in the grid
			
			unsigned int mappos = getBoxinMap(floor(xmin / size) + floor(zmin / size) * lwh.x + floor(ymin / size) * lwh.x * lwh.y);
			
			while ((mappos = getNextTriangle(mappos)) != 0) // loop till no more tests
			{
				const unsigned int index = getTriangleData(mappos);

				// search the map for the triangle
				/*
				tricheck = false;
				
				unsigned int vtlookup = viewedtri[ceil(index / NUMPERKEY)];
				
				if (vtlookup == 0)
					tricheck = false;
				else
				{
					// follow the linklist

					while ((vtlookup = viewedtri[vtlookup]) != 0 && viewedtri[vtlookup + 1] != index)
					{}

					// check why the while loop was exited
					
					tricheck = (viewedtri[vtlookup + 1] == index) ? true : false;
				}
				
				// the triangle was not found

				if (!tricheck)*/
				{
					// check for collisions with the triangle

					tri.v1 = trimesh[indices[index]].position;

					tri.v2 = trimesh[indices[index + 1]].position;

					tri.v3 = trimesh[indices[index + 2]].position;

					rayTriangleIntersection(tri, ray, collision, raydistance);

					if (collision && raydistance < collisiondistance)
					{
						collisionfound = true;

						collisionindex = index;

						collisiondistance = raydistance;

						collisionpt = linep[0] + ray.direction * raydistance;
					}
					/*
					// change the end of the link list for this triangle

					viewedtri[vtlookup] = viewedtrisize;

					// add the triangle to the end of the list

					if (viewedtrisize + 2 < MAXTRIANGLEMAP)
					{
						viewedtri[viewedtrisize] = 0; // store the triangle so we do not check it twice

						viewedtri[viewedtrisize + 1] = index;

						viewedtrisize += 2; // update the size
					}
					else
						return false;*/
				}
			}

			// a collision has occured and the closest point was found

			if (collisionfound)
				return true;
		}

		// reset fin

		posupdate = false;

		// reset distance

		mindistance = FLT_MAX;

		// loop for the 3 sides where collisions are possible

		[unroll(3)]
		for (unsigned int i = 0; i < 3; i++)
		{
			{
				tmppos = pos;

				tmppos[i] += halfsize * dir[i];

				if (linePlaneCollision(distance, linexyz[0], linexyz[1], float3(i == 0, i == 1, i == 2), tmppos))
				{
					posupdate = true; // the position has been updated

					if (abs(distance - mindistance) < DISTANCERE) // collide at the same point with the square (on an edge)
						nextpos[i] += size * dir[i]; // update the position
					else if (distance < mindistance)
					{
						mindistance = distance; // update the distance

						nextpos = pos; // set nextpos

						nextpos[i] += size * dir[i]; // update the position
					}
				}
			}
		}

		pos = nextpos;
	}

	return false;
}

float shadowCalc(float4 position, float3 normal)
{
	// iterate over the area light (normal polygon of point numshadowrays)

	float3 shadowline[2];
	
	shadowline[0] = (float3)(position);

	const float anglebetweenpoints = 2.f * PI / numshadowrays;

	unsigned int collisionindex;
	
	float3 collisionpt;

	float shadows = 0;

	float2 seed = float2(position.x + position.y, position.z / position.y);

	float lightradius = 10;

	for (float i = 0; i < numshadowrays; i++)
	{
		// calculate point position

		lightradius = rand(seed, 0, 10.f);

		float p = acos(2.0 * rand(seed, 0, 1) - 1.0);

		float u = cos(p);

		float theta = 2.0 * M_PI * rand(seed, 0, 1);

		float x = sqrt(1 - u * u) * cos(theta);

		float y = sqrt(1 - u * u) * sin(theta);

		float z = u;

		// CHECK
		/*float x = rand(seed, 0, lightradius);

		float y = rand(seed, 0, (lightradius - x));

		float z = sqrt(lightradius * lightradius - (x * x + y * y));

		// generate signs
		
		x *= randsign(seed);

		y *= randsign(seed);

		z *= randsign(seed);*/
		
		//shadowline[1] = shadowline[0] - float3(0, 0, -1) * 50;//lightpos + float3(x, y, z);
			
		shadowline[1] = lightpos + float3(lightradius * cos(anglebetweenpoints * i), 0, lightradius * sin(anglebetweenpoints * i));

		// generate a hemisphere of rays around the collision

		// check for collisions

		if (linetoSquare(shadowline, collisionindex, collisionpt))
			shadows++;
	}
	
	return 1.f - shadows / numshadowrays;
}

void reflectionRefractionCalculation(inout float4 color, float4 position, float3 normal)
{
	float3 refline[2];

	refline[0] = (float3)position;

	unsigned int collisionindex;

	float3 collisionpt;

	float relfections = 0;

	float3 direction = position - campos;

	bool updated = true; // make sure a collision existed in the previous check before moving ahead with the loop

	[unroll(3)]
	for (unsigned int relfections = 0; updated && relfections < 10 && 0 < sum(color); relfections++)
	{
		// calculate reflected ray

		refline[1] = refline[0] + normalize(reflect(direction, normal)) * RAYDISTANCE; //normalize(refract(direction, normal, 10)) * RAYDISTANCE;//normalize(reflect(direction, normal)) * RAYDISTANCE;

		direction = refline[1] - refline[0];

		if (linetoSquare(refline, collisionindex, collisionpt))
		{
			// convert vertex normal to face normal

			normal = vertexNormaltoFaceNormal(
				trimesh[indices[collisionindex]].position,
				trimesh[indices[collisionindex + 1]].position,
				trimesh[indices[collisionindex + 2]].position,
				trimeshn[indices[collisionindex]].normal,
				trimeshn[indices[collisionindex + 1]].normal,
				trimeshn[indices[collisionindex + 2]].normal
				);

			const float2 uv = getTexCoord(
				trimesh[indices[collisionindex]].position,
				trimesh[indices[collisionindex + 1]].position,
				trimesh[indices[collisionindex + 2]].position,
				collisionpt,
				trimeshuv[indices[collisionindex]].uv,
				trimeshuv[indices[collisionindex + 1]].uv,
				trimeshuv[indices[collisionindex + 2]].uv
				);

			color *= diffusemap.Sample(linearsampler, uv);

			// update position

			refline[0] = collisionpt;

			// set updated to true

			updated = true;
		}
		else
		{
			color *= float4(1.0f, 1.0f, 0.0f, 1.0f);

			// set updated to false

			updated = false;
		}
	}
}

float4 phongCalc(float3 normal, float4 ambientc, float4 diffusec, float4 specularc, float shininessc, float4 lighcolor, float3 lightvec, float3 camvec)
{
	float4 ambientCalc = ambientc;

	float4 diffuseCalc = diffusec * saturate(dot(normal, lightvec));

	float4 specularCalc = specularc * pow(saturate(dot(reflect(lightvec, normal), camvec)), shininessc);

	return ambientCalc + (diffuseCalc + specularCalc) * lighcolor;
}

float4 rayTrace(float4 position, float3 normal, float4 color)
{
	// run a standard phong shader

	// launch shadow ray from primary ray

	float shadow = shadowCalc(position, normal);

	// launch reflection ray from primary ray

	//float4 specularr = specular;
	//float2 r = float2(position.x + position.y, position.z / position.y);
	//float tmp = rand(r);
	//if (tmp < .2)
		//return float4(1, 0, 0, 1);

	//return float4(tmp, tmp, tmp, 1.f);
	reflectionRefractionCalculation(color, position, normal);
	//return float4(shadow, shadow, shadow, 1.f);// 
	return phongCalc(normal, ambient, color * shadow, specular, shininess, float4(1, 1, 1, 1), normalize(lightpos - position), normalize(campos - position));
	//return color * shadow;
	//return phongCalc(normal, ambient, color * shadow, specular, shininess, float4(1, 1, 1, 1), normalize(lightpos - position), normalize(campos - position));//color;
}

float4 mainPS(PSINPUT In) : SV_TARGET
{
	float4 diffuse_color = diffuse;

	if (texturei)
		diffuse_color *= diffusemap.Sample(linearsampler, In.texcoord);

	return rayTrace(In.worldpos, In.normal, diffuse_color);
}