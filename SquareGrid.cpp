#include <vector>
#include <d3dx9math.h>
#include "SquareGrid.h"
#include "Collisions.h"

using namespace std;

#define MAX_VECTOR D3DXVECTOR3(FLT_MAX, FLT_MAX, FLT_MAX)

const D3DXVECTOR3 cubeverts[] = {
	D3DXVECTOR3(-1, 1, 1), D3DXVECTOR3(1, 1, 1), D3DXVECTOR3(1, -1, 1), D3DXVECTOR3(-1, -1, 1),
	D3DXVECTOR3(-1, 1, -1), D3DXVECTOR3(1, 1, -1), D3DXVECTOR3(1, -1, -1), D3DXVECTOR3(-1, -1, -1),

	D3DXVECTOR3(-1, 1, -1), D3DXVECTOR3(-1, 1, 1), D3DXVECTOR3(-1, -1, 1), D3DXVECTOR3(-1, -1, -1),
	D3DXVECTOR3(1, 1, -1), D3DXVECTOR3(1, 1, 1), D3DXVECTOR3(1, -1, 1), D3DXVECTOR3(1, -1, -1),

	D3DXVECTOR3(-1, 1, -1), D3DXVECTOR3(1, 1, -1), D3DXVECTOR3(1, 1, 1), D3DXVECTOR3(-1, 1, 1),
	D3DXVECTOR3(-1, -1, -1), D3DXVECTOR3(1, -1, -1), D3DXVECTOR3(1, -1, 1), D3DXVECTOR3(-1, -1, 1)
};

const D3DXVECTOR3 cubenormal[] = {
	D3DXVECTOR3(0, 0, 1),
	D3DXVECTOR3(0, 0, -1),
	D3DXVECTOR3(-1, 0, 0),
	D3DXVECTOR3(1, 0, 0),
	D3DXVECTOR3(0, 1, 0),
	D3DXVECTOR3(0, 1, 0)
};

struct VECTOR3ui
{
	unsigned int x, y, z;

	VECTOR3ui::VECTOR3ui()
	{

	}

	VECTOR3ui::VECTOR3ui(unsigned int px, unsigned int py, unsigned int pz)
	{
		x = px;

		y = py;

		z = pz;
	}
};

struct Edge
{
	D3DXVECTOR3 v0, v1;

	Edge::Edge()
	{

	}

	Edge::Edge(D3DXVECTOR3 v0p, D3DXVECTOR3 v1p)
	{
		v0 = v0p;

		v1 = v1p;
	}
};

SquareGrid::SquareGrid()
{

}

SquareGrid::~SquareGrid()
{

}

/*
Check if a point is whithin the grid
*/

bool SquareGrid::withinBounds(D3DXVECTOR3 pos)
{
	// transform the coordinate

	D3DXVECTOR3 squarecoord = pos - start;

	// get the coord in the grid

	return floor(squarecoord.x / size) * size >= 0 && floor(squarecoord.y / size) * size >= 0 && floor(squarecoord.z / size) * size >= 0;
}

/*
Get the 3D coord of the position in the map on the grid.
*/

D3DXVECTOR3 SquareGrid::getLocfromPos(unsigned int loc)
{
	D3DXVECTOR3 pos;

	pos.y = floor((float)loc / (float)(length * width));

	unsigned int yindex = pos.y * length * width;

	pos.z = floor((float)(loc - yindex) / (float)length);

	unsigned int zindex = pos.z * length;

	pos.x = loc - yindex - zindex;

	pos *= size;

	// undo the transform

	pos += start;

	return pos;
}

/*
Get the 3D position of the box that contains this point.
*/

D3DXVECTOR3 SquareGrid::getBoxPosatLoc(D3DXVECTOR3 pos)
{
	// transform the coordinate

	D3DXVECTOR3 squarecoord = pos - start;

	// get the coord in the grid

	squarecoord.x = (unsigned int)floor(squarecoord.x / size) * size;
	squarecoord.y = (unsigned int)floor(squarecoord.y / size) * size;
	squarecoord.z = (unsigned int)floor(squarecoord.z / size) * size;

	// undo the transform

	squarecoord += start;

	return squarecoord;
}

/*
Get the position of a point in the grid.
*/

unsigned int SquareGrid::getPosatLoc(D3DXVECTOR3 pos)
{
	D3DXVECTOR3 transcoord = pos - start;

	// get the coord in the grid

	unsigned int x = (unsigned int)floor(transcoord.x / size);
	unsigned int y = (unsigned int)floor(transcoord.y / size);
	unsigned int z = (unsigned int)floor(transcoord.z / size);

	// find the unsigned int version of this coord

	return x + z * length + y * length * width;
}

/*
Get the position of a point in the grid.
*/

unsigned int SquareGrid::getPosatLoc(unsigned int x, unsigned int y, unsigned int z)
{
	// find the unsigned int version of this coord

	return x + z * length + y * length * width;
}

/*
Move by a certain number of spots in the x, y, or z directions.
*/

unsigned int SquareGrid::moveLoc(unsigned int loc, D3DXVECTOR3 move)
{
	return loc + getPosatLoc(move.x, move.y, move.z);
}

/*
Create the grid for triangle square collision.
*/

void SquareGrid::createGrid(D3DXVECTOR3 startp, float sizep, unsigned int lengthp, unsigned int widthp, unsigned int heightp)
{
	// copy the elements for later use

	start = startp;

	size = sizep;

	length = lengthp;

	width = widthp;

	height = heightp;

	// create the grid

	newGrid(length * width * height, 100);
}

/*
Remake the triangle out of boxes and add the triangle to the grid.
*/

void SquareGrid::triangletoBox(D3DXVECTOR3* tri, unsigned int trinum)
{
	Edge* plane[3];

	unsigned int numedges[3];

	// find the minimum values

	D3DXVECTOR3 min = tri[0];

	// find the maximum values

	D3DXVECTOR3 max = tri[0];

	for (int i = 1; i < 3; i++)
		for (int j = 0; j < 3; j++)
		{
			// adjust min value

			if (tri[i][j] < min[j])
				min[j] = tri[i][j];

			// adjust max value

			if (max[j] < tri[i][j])
				max[j] = tri[i][j];
		}

	// create edge variables

	for (int i = 0; i < 3; i++)
	{
		if ((max[i] - start[i]) / size != (min[i] - start[i]) / size)
			numedges[i] = floor((max[i] - start[i]) / size) - ceil((min[i] - start[i]) / size) + 1;
		else
			numedges[i] = 1;

		plane[i] = new Edge[numedges[i]];
	}

	// loop through to test intersections

	D3DXVECTOR3 line[2];

	D3DXVECTOR3 cubepos;

	VECTOR3ui index;

	for (unsigned int i = 0; i < 3; i++)
	{
		for (int j = 0; j < numedges[i]; j++)
		{
			if (trianglePlaneCollision(line, tri, D3DXVECTOR3(i == 0, i == 1, i == 2), D3DXVECTOR3(i == 0, i == 1, i == 2) * (start[i] + (j + ceil((min[i] - start[i]) / size)) * size)))
				plane[i][j] = Edge(line[0], line[1]);
			else // the line infinitely intersects the triangle
				plane[i][j] = Edge(MAX_VECTOR, MAX_VECTOR);
		}
	}

	// now create the boxes

	vector <unsigned int> queue;

	queue.push_back(getPosatLoc(tri[0]));

	add(queue[0], trinum); // add the box to the grid

	D3DXVECTOR3 tmppos;

	D3DXVECTOR3 queueloc; // the 3D location of the 0 element in the queue

	unsigned int tmpindex; // the index of the square being checked

	float planeindex;

	while (queue.size() != 0) // loop until all possible boxes are checked
	{
		// check for collisions in each of the walls

		for (int i = 0; i < 3; i++)
		{
			// find the plane

			queueloc = getLocfromPos(queue[0]);

			planeindex = floor((queueloc[i] - start[i]) / size) - ceil((min[i] - start[i]) / size);

			for (int j = 0; j < 2; j++)
			{
				tmppos = D3DXVECTOR3(0, 0, 0);

				tmppos[i] = j == 0.f ? -1.f : 1.f;

				tmpindex = moveLoc(queue[0], tmppos);

				tmppos = getLocfromPos(tmpindex); // fixes rounding 

				if (withinBounds(tmppos) && // make sure the position is within the grid
					!exists(tmpindex, trinum) && // check that the square is not added in that position already
					0 <= planeindex && planeindex < numedges[i] // make sure the planeindex is whithin bounds
					)
				{
					if (plane[i][(unsigned int)planeindex].v0 == MAX_VECTOR && plane[i][(unsigned int)planeindex].v1 == MAX_VECTOR || // check if the intersection is infinite
						lineSquareCollision(D3DXVECTOR2(plane[i][(unsigned int)planeindex].v0[(i + 1) % 3], plane[i][(unsigned int)planeindex].v0[(i + 2) % 3]), // test the square intersects the line
						D3DXVECTOR2(plane[i][(unsigned int)planeindex].v1[(i + 1) % 3], plane[i][(unsigned int)planeindex].v1[(i + 2) % 3]),
						D3DXVECTOR2(tmppos[(i + 1) % 3] + size / 2.f, tmppos[(i + 2) % 3] + size / 2.f), size))
					{
						add(tmpindex, trinum); // add the box to the grid

						queue.push_back(tmpindex); // add the box to the queue
					}
				}

				planeindex++;
			}
		}

		// if no new boxes were added, check diagonals

		// remove from queue

		queue.erase(queue.begin());
	}

	// free all data

	for (int i = 0; i < 3; i++)
		delete plane[i];
}

/*
Get the start position of the grid.
*/

const D3DXVECTOR3* SquareGrid::getStart()
{
	return &start;
}

/*
Get the length / width / height of each square on the grid.
*/

const float SquareGrid::getSize()
{
	return size;
}