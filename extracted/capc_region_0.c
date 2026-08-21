#include <stdio.h>
#define N 10000000

void capc_region_0(double (* restrict A), double (* restrict B))
{
    int i;
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(N - i);
    }

}
