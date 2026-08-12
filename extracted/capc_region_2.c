#include <stdio.h>
#define SIZE 1000000000

void capc_region_2(double (* restrict D), double (* restrict A), double (* restrict B))
{
    int i;
	for (i=0; i<SIZE; ++i)
	{
		D[i]=A[i]-B[i];
	}

}
