#include <stdio.h>
#define SIZE 1000000000

void capc_region_1(double (* restrict C), double (* restrict A), double (* restrict B))
{
    int i;
	for (i=0; i<SIZE; ++i)
	{
		C[i]=A[i]+B[i];
	}

}
