#include <stdio.h>
#define SIZE 1000000000

void capc_region_3(double (* restrict E), double (* restrict A), double (* restrict B))
{
    int i;
	for (i=0; i<SIZE; ++i)
	{
		E[i]=A[i]*B[i];
	}

}
