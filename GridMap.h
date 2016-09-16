#ifndef GRIDMAP_H
#define GRIDMAP_H

#include <vector>

using namespace std;

class GridMap
{
public:

	GridMap();

	GridMap(unsigned int maxnump, unsigned int numperlinkp);

	~GridMap();

	void newGrid(unsigned int maxnump, unsigned int numperlinkp);

	bool add(unsigned int boxnum, unsigned int trinum);

	void at(unsigned int boxnum, unsigned int trinum);

	bool exists(unsigned int boxnum, unsigned int trinum);

	const unsigned int getMaxNum();

	const unsigned int getNumPerLink();

	const unsigned int* getGrid();

	const unsigned int getGridSize();

	void resize(unsigned int newsize);

private:

	unsigned int atGrid(unsigned int i);

	unsigned int getNextTriangle(unsigned int pos);

	unsigned int getTriangleData(unsigned int pos);

	unsigned int getBoxinMap(unsigned int boxnum);

	unsigned int maxnum;

	unsigned int numperlink;

	vector<unsigned int> gridmap;
};

#endif