#include "GridMap.h"

/*
The way the map works is as follows:
Link list size maxnum / numperlink to boxes link list.  The key for each box is calculated by boxnum / numperlink.
Boxes link list stores the location of the next data in the list in its first unsigned int.  The second unsigned int holds the start of a triangle link list for that box.  The third unsigned int holds the box number.
Triangle link list stores the location of the next data in the list in its first unsigned int.  The second unsigned int holds the triangle number.
If any of these link lists have a location of 0, the link list is ended.
*/

GridMap::GridMap()
{

}

GridMap::GridMap(unsigned int maxnump, unsigned int numperlinkp)
{
	maxnum = maxnump;

	numperlink = numperlinkp;

	gridmap.resize(maxnum / numperlink); // fills the link list with zeros
}

GridMap::~GridMap()
{

}

/*
Change the size of the grid and make a new one
*/

void GridMap::newGrid(unsigned int maxnump, unsigned int numperlinkp)
{
	maxnum = maxnump;

	numperlink = numperlinkp;

	gridmap.resize(maxnum / numperlink); // resize the grid

	// clear out the data in the grid

	for (unsigned int i = 0; i < maxnum / numperlink; i++)
		gridmap[i] = 0;
}

/*
Add an element to the map.
*/

bool GridMap::add(unsigned int boxnum, unsigned int trinum)
{
	if (maxnum < boxnum)
		return false;

	const unsigned int linklistsize = maxnum / numperlink;

	unsigned int pos = boxnum / numperlink; // find the start of the link list using the key

	while (pos < linklistsize && gridmap[pos] != 0 || pos >= linklistsize && gridmap[pos] != 0 && gridmap[pos + 2] != boxnum) // loop till the while reaches the end of the link list or finds the box
	{
		// move to the next position

		pos = gridmap[pos];
	}

	if (pos < linklistsize && gridmap[pos] == 0 || pos >= linklistsize && gridmap[pos] == 0 && gridmap[pos + 2] != boxnum) // box is not found
	{
		gridmap[pos] = gridmap.size(); // add the position of the next box to the link list

		pos = gridmap.size(); // update the position

		// add the box to the end of the list

		gridmap.push_back(0); // end off the link list

		gridmap.push_back(0); // end off the link list

		gridmap.push_back(boxnum); // store the box
	}

	pos++; // update the position

	// scale this link list till the end or if the triangle already exists

	while (gridmap[pos] != 0 && gridmap[pos + 1] != trinum)
	{
		// move to the next position

		pos = gridmap[pos];
	}

	if (gridmap[pos + 1] == trinum)
		return true;

	// end of list reached

	gridmap[pos] = gridmap.size(); // add the position of the next triangle to the link list

	// add the triangle to the end of the list

	gridmap.push_back(0); // end off the link list

	gridmap.push_back(trinum); // add the triangle

	return false;
}

/*
Check if an element exists in the triangle list.
*/

bool GridMap::exists(unsigned int boxnum, unsigned int trinum)
{
	if (maxnum < boxnum)
		return false;

	const unsigned int linklistsize = maxnum / numperlink;

	unsigned int pos = boxnum / numperlink; // find the start of the link list using the key

	while (pos < linklistsize && gridmap[pos] != 0 || pos >= linklistsize && gridmap[pos] != 0 && gridmap[pos + 2] != boxnum) // loop till the while reaches the end of the link list or finds the box
	{
		// move to the next position

		pos = gridmap[pos];
	}

	if (pos < linklistsize && gridmap[pos] == 0 || pos >= linklistsize && gridmap[pos] == 0 && gridmap[pos + 2] != boxnum) // box is not found
		return false;

	pos++; // update the position

	// scale this link list till the end or if the triangle already exists

	while (gridmap[pos] != 0 && gridmap[pos + 1] != trinum)
	{
		// move to the next position

		pos = gridmap[pos];
	}

	if (gridmap[pos + 1] == trinum) // triangle found
		return true;

	return false; // end of list reached
}

/*
Get the maximum number that could be stored in the grid
*/

const unsigned int GridMap::getMaxNum()
{
	return maxnum;
}

/*
Get the number of boxes stored per uint in the link list.
*/

const unsigned int GridMap::getNumPerLink()
{
	return numperlink;
}

/*
Retrieve a pointer to where the grid data is.
*/

const unsigned int* GridMap::getGrid()
{
	return &(gridmap[0]);
}

/*
Get the size of the grid.
*/

const unsigned int GridMap::getGridSize()
{
	return gridmap.size();
}

/*
Resize the map if it is larger than its current size.
*/

void GridMap::resize(unsigned int newsize)
{
	if (gridmap.size() < newsize)
		gridmap.resize(newsize);
}

/**
FOR DEBUG ONLY
**/

/*
get a value at the i location in the grid as if it were not 2 dimensions
*/

unsigned int GridMap::atGrid(unsigned int i)
{
	return i < gridmap.size() ? gridmap[i] : 0;
}


/*
Get the next loctaion of a triangle in the map.
*/

unsigned int GridMap::getNextTriangle(unsigned int pos)
{
	return atGrid(pos);
}

/*
Get the triangle number by moving past the header
*/

unsigned int GridMap::getTriangleData(unsigned int pos)
{
	return atGrid(pos + 1);
}

/*
Get the loctaion of a box in the map.
*/

unsigned int GridMap::getBoxinMap(unsigned int boxnum)
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
