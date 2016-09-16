#include <algorithm> 
#include <d3dx9math.h>

using namespace std;

#define LPERROR .00001 // error for line plane collision

#define LPERROR .00001 // error for a line being considered a point

/*
Get the sign of a float.
*/

float getSign(float n)
{
	if (n == 0)
		return 0;

	return n < 0 ? -1.f : 1.f;
}

/*
Collision between a line and a plane.
Based on https://www.cs.princeton.edu/courses/archive/fall00/cs426/lectures/raycast/sld017.htm.
*/

bool linePlaneCollision(D3DXVECTOR3* intersection, D3DXVECTOR3 v0, D3DXVECTOR3 v1, D3DXVECTOR3 normal, D3DXVECTOR3 point)
{
	// calculate d

	float d = -D3DXVec3Dot(&point, &normal);

	D3DXVECTOR3 dir = v1 - v0;

	// calculate t

	float t = -(D3DXVec3Dot(&v0, &normal) + d) / D3DXVec3Dot(&dir, &normal);

	if (!isfinite(t) || 1.f + LPERROR < t || t < 0.f - LPERROR)
		return false;

	// get the intersection point

	*intersection = v0 + t * dir;

	return true;
}

/*
Collision between a triangle and a plane.
Return the collision points.
*/

bool trianglePlaneCollision(D3DXVECTOR3* line, D3DXVECTOR3* tri, D3DXVECTOR3 normal, D3DXVECTOR3 point)
{
	D3DXVECTOR3 intersection; // used in the collision test

	unsigned int fill = 0; // how many elements have been added to the line

	for (int i = 0; i < 3 && fill != 2; i++)
	{
		if (linePlaneCollision(&intersection, tri[i % 3], tri[(i + 1) % 3], normal, point))
		{
			if (fill == 2) // if two points are the same
			{
				bool found = false;

				// search for the similar value

				for (int i = 0; i < 2 && !found; i++)
				{
					if (line[i] == intersection)
						found = true;
				}

				// insert the new value if needed

				if (found)
					line[0] = intersection;
			}
			else
			{
				line[fill] = intersection;

				fill++;
			}
		}
	}

	if (fill == 1)
	{
		line[1] = line[0]; // a point

		return true;
	}
	else if (fill == 2)
		return true;

	return false;
}


/*
Is a vert inside of a square
*/

bool vertInSquare(D3DXVECTOR2 v0, D3DXVECTOR2 squarecenter, float size)
{
	if (squarecenter.x - size / 2.f <= v0.x && v0.x <= squarecenter.x + size / 2.f &&
		squarecenter.y - size / 2.f <= v0.y && v0.y <= squarecenter.y + size / 2.f)
		return true;

	return false;
}

/*
Point square collision detection.
*/

bool pointSquareCollision(D3DXVECTOR2 pt, D3DXVECTOR2 squarecenter, float size)
{
	const float halfsize = size / 2.f;

	return pt.x <= squarecenter.x + halfsize
		&& squarecenter.x - halfsize <= pt.x
		&& pt.y <= squarecenter.y + halfsize
		&& squarecenter.y - halfsize <= pt.y;
}

/*
Line square collision deteciton.
Based on http://stackoverflow.com/questions/99353/how-to-test-if-a-line-segment-intersects-an-axis-aligned-rectange-in-2d
*/

bool lineSquareCollision(D3DXVECTOR2 v0, D3DXVECTOR2 v1, D3DXVECTOR2 squarecenter, float size)
{
	// make certain the line entered is not a point instead

	if (abs(v0.x - v1.x) < LPERROR && abs(v0.y - v1.y) < LPERROR)
		return pointSquareCollision((v0 + v1) / 2.f, squarecenter, size);

	const float halfsize = size / 2.f;

	// check if an intersection is possible

	if (squarecenter.x + halfsize < v0.x && squarecenter.x + halfsize < v1.x)
		return false;
	else if (v0.x < squarecenter.x - halfsize && v1.x < squarecenter.x - halfsize)
		return false;
	else if (squarecenter.y + halfsize < v0.y && squarecenter.y + halfsize < v1.y)
		return false;
	else if (v0.y < squarecenter.y - halfsize && v1.y < squarecenter.y - halfsize)
		return false;

	// check if the line intersects the square

	bool signchange = false;
	float tmpsign;
	float sign;

	// setup the vars for the equation

	const float A = v1.y - v0.y;
	const float B = v0.x - v1.x;
	const float C = v1.x * v0.y - v0.x * v1.y;

	const D3DXVECTOR2 squaresign[] = {
		D3DXVECTOR2(-1, 1), D3DXVECTOR2(1, 1), D3DXVECTOR2(1, -1), D3DXVECTOR2(-1, -1)
	};

	// loop through the verts of the square
	// F(x y) = (y2-y1)x + (x1-x2)y + (x2*y1-x1*y2)

	for (int i = 0; i < 4 && !signchange; i++)
	{//getSign copysign
		tmpsign = getSign(A * (squarecenter.x + halfsize * squaresign[i].x)
			+ B * (squarecenter.y + halfsize * squaresign[i].y)
			+ C);

		if (tmpsign == 0)
			signchange = true;
		else if (i == 0)
			sign = tmpsign;
		else if (sign != tmpsign)
			signchange = true;
	}

	if (!signchange)
		return false;

	return true;
}
