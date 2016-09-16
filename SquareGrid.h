#ifndef PLANESLICE_H
#define PLANESLICE_H

#include <vector>
#include <d3dx9math.h>
#include "GridMap.h"

using namespace std;

class SquareGrid : public GridMap
{
public:

	SquareGrid();

	~SquareGrid();

	void createGrid(D3DXVECTOR3 startp, float sizep, unsigned int lengthp, unsigned int widthp, unsigned int heightp);

	void triangletoBox(D3DXVECTOR3* tri, unsigned int trinum);

	const D3DXVECTOR3* getStart();

	const float getSize();

	// TMP IN PUBLIC

	D3DXVECTOR3 getLocfromPos(unsigned int loc);

private:

	// methods

	bool withinBounds(D3DXVECTOR3 pos);

	D3DXVECTOR3 getBoxPosatLoc(D3DXVECTOR3 pos);

	unsigned int getPosatLoc(D3DXVECTOR3 pos);

	unsigned int getPosatLoc(unsigned int x, unsigned int y, unsigned int z);

	unsigned int moveLoc(unsigned int loc, D3DXVECTOR3 move);

	// variables

	vector<unsigned int> gridexport;

	D3DXVECTOR3 start;

	float size;

	unsigned int length, width, height;
};

#endif