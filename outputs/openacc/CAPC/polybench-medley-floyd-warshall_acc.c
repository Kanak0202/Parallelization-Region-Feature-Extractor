#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2800

int path[N][N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            path[i][j] = (i * j) % 7 + 1;

            if ((i + j) % 13 == 0 ||
                (i + j) % 7 == 0 ||
                (i + j) % 11 == 0)
            {
                path[i][j] = 999;
            }
        }
    }
}

void kernel_floyd_warshall()
{
    int i, j, k;

    for (k = 0; k < N; k++)
    {
#pragma capc profitability_region begin
#pragma acc parallel loop collapse(2) copy(path[0:N][0:N])
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                path[i][j] =
                    path[i][j] < path[i][k] + path[k][j]
                    ? path[i][j]
                    : path[i][k] + path[k][j];
            }
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_floyd_warshall();

    /* Prevent complete dead-code elimination. */
    printf("%d\n", path[0][0]);

    return 0;
}