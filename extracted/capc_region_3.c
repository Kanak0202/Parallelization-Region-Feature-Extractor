#include <stdio.h>
#define N 10000000

void capc_region_3(double (* restrict C), double (* restrict A), double (* restrict B))
{
    int i;
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }

}
