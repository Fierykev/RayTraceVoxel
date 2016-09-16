#ifndef COLLISIONS_H
#define COLLISIONS_H

#include <d3dx9math.h>

bool linePlaneCollision(D3DXVECTOR3* intersection, D3DXVECTOR3 v0, D3DXVECTOR3 v1, D3DXVECTOR3 normal, D3DXVECTOR3 point);

bool trianglePlaneCollision(D3DXVECTOR3* line, D3DXVECTOR3* tri, D3DXVECTOR3 normal, D3DXVECTOR3 point);

bool vertInSquare(D3DXVECTOR2 v0, D3DXVECTOR2 squarecenter, float size);

bool lineLineCollision(D3DXVECTOR2 v00, D3DXVECTOR2 v01, D3DXVECTOR2 v10, D3DXVECTOR2 v11);

bool lineSquareCollision(D3DXVECTOR2 v0, D3DXVECTOR2 v1, D3DXVECTOR2 squarecenter, float size);

#endif