#include <stdio.h>
#define N 10000000

void capc_region_1(double (* restrict A))
{
    int i;
    for (i = 0; i < N; i++)
    {
        A[i] = 1.5 * A[i];
    }

}
