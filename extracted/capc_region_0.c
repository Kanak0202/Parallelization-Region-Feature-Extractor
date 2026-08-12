#include <stdio.h>
#define SIZE 1000000000

void capc_region_0(double (* restrict A), double (* restrict B))
{
    int i;
	for (i=0; i<SIZE; ++i)
	{
		A[i] = (double)i;
		B[i] = (double)(i+1);
	}

}
