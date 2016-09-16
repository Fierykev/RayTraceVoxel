Texture1D<unsigned int> gridmap : register(t5); // stores the grid map

/*
Get the loctaion of a box in the map.
*/

unsigned int getBoxinMap(unsigned int maxnum, unsigned int numperlink, unsigned int boxnum)
{
	if (maxnum <= boxnum || numperlink == 0)
		return 0;
	
	//const unsigned int linklistsize = maxnum / numperlink;

	//unsigned int pos = (unsigned int)boxnum;// / numperlink; // find the start of the link list using the key
	/*
	[unroll(10)]
	while (pos < linklistsize && gridmap[pos] != 0 || pos >= linklistsize && gridmap[pos] != 0 && gridmap[pos + 2] != boxnum) // loop till the while reaches the end of the link list or finds the box
	{
		// move to the next position

		pos = gridmap[pos];
	}

	if (pos < linklistsize && gridmap[pos] == 0 || pos >= linklistsize && gridmap[pos] == 0 && gridmap[pos + 2] != boxnum) // box is not found
		return 0;

	return pos + 1;*/

	return 0;
}

/*
Get the next loctaion of a triangle in the map.
*/

unsigned int getNextTriangle(unsigned int pos)
{
	if (gridmap[pos] != 0)
	{
		// move to the next position

		return gridmap[pos];
	}

	return 0;
}


/*
Get the triangle number by moving past the header
*/
/*
unsigned int getTriangleData(unsigned int pos)
{
	return pos + 1;
}*/