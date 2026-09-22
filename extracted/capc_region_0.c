#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#define n 17

void capc_region_0(float (* restrict A)[17][17], float (* restrict B)[17][17])
{
    int i;
    int j;
    int k;
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				A[i][j][k] = B[i][j][k] = (float) (i + j + (n-k))* 10 / (n);

}
