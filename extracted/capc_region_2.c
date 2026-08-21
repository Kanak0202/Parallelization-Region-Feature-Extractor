#include <stdio.h>
#define N 10000000

void capc_region_2(double (* restrict B))
{
    int i;
    for (i = 0; i < N; i++)
    {
        B[i] = B[i] + 2.0;
    }

}
