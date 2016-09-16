#define swap(a, b) {float t; t = a; a = b; b= t;}

cbuffer SquareGridCB : register(b3)
{
	float3 lwh : packoffset(c0);
	float size : packoffset(c0.a);
	float3 start : packoffset(c1);
	float maxnum : packoffset(c1.a);
	float numperlink : packoffset(c2);
}

float4 finalGraph(float x0, float x1, float y0, float y1, float z0, float z1, float size, float4 color)
{
	const float xmin = min(x0, x1);

	const float ymin = min(y0, y1);

	const float zmin = min(z0, z1);

	// check the spot in the grid
	
	//unsigned int pos = getBoxinMap(0, 0, 0);// maxnum, numperlink, floor(xmin / size) + floor(ymin / size) * lwh.x + floor(zmin / size) * lwh.x * lwh.y);
	/*
	if ((pos = getNextTriangle(pos)) != 0) // loop till no more tests
	{
		return float4(1, 1, 1, 1);
	}*/

	return float4(numperlink / 10, numperlink / 10, numperlink / 10, 0);
}

float4 yzGraph(float3 linexyz[2], float x0, float x1, float y0, float y1, float m2, float zinc, float size, float4 color)
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
		color = finalGraph(x0, x1, y0, y1, z0, z, size, color);

		for (; 0 < distance; distance--)
		{
			color = finalGraph(x0, x1, y0, y1, z, z + zinc * size, size, color);

			z += zinc * size;
		}

		if (floor(z0) != floor(z1)) // not in the same square
			color = finalGraph(x0, x1, y0, y1, z, z1, size, color);
	}
	else
		color = finalGraph(x0, x1, y0, y1, z0, z1, size, color);

	return color;
}

float4 xyzGraph(float3 linexyz[2], float x0, float x1, float m, float m2, float yinc, float zinc, float size, float4 color)
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
		color = yzGraph(linexyz, x0, x1, y0, y, m2, zinc, size, color);

		for (; 0 < distance; distance--)
		{
			color = yzGraph(linexyz, x0, x1, y, y + yinc * size, m2, zinc, size, color);

			y += yinc * size;
		}

		if (floor(y0) != floor(y1)) // not in the same square
			color = yzGraph(linexyz, x0, x1, y, y1, m2, zinc, size, color);
	}
	else
		color = yzGraph(linexyz, x0, x1, y0, y1, m2, zinc, size, color);

	return color;
}

float4 linetoSquare(float3 linep[2], float4 color)
{
	// move the linexyz into the grid

	float3 linexyz[2];

	linexyz[0] = linep[0] - start;

	linexyz[1] = linep[1] - start;

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
		color = xyzGraph(linexyz, linexyz[0].x, linexyz[1].x, m, m2, yinc, zinc, size, color);
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
			color = xyzGraph(linexyz, linexyz[0].x, x, m, m2, yinc, zinc, size, color);

			for (; 0 < distance; distance--)
			{
				color = xyzGraph(linexyz, x, x + xinc * size, m, m2, yinc, zinc, size, color);

				x += xinc * size;
			}

			color = xyzGraph(linexyz, x, linexyz[1].x, m, m2, yinc, zinc, size, color);
		}
		else
			color = xyzGraph(linexyz, linexyz[0].x, linexyz[1].x, m, m2, yinc, zinc, size, color);
	}
	
	return color;
}