#include <stdio.h>
#include <stdlib.h>

#define N 2000
#define TSTEPS 500

double A[N][N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] =
                ((double)i * (j + 2) + 2.0) / N;
        }
    }
}

void kernel_seidel_2d()
{
    int t, i, j;

    for (t = 0; t < TSTEPS; t++)
    {
        for (i = 1; i < N - 1; i++)
        {
            for (j = 1; j < N - 1; j++)
            {
                A[i][j] =
                    (A[i - 1][j - 1]
                     + A[i - 1][j]
                     + A[i - 1][j + 1]
                     + A[i][j - 1]
                     + A[i][j]
                     + A[i][j + 1]
                     + A[i + 1][j - 1]
                     + A[i + 1][j]
                     + A[i + 1][j + 1])
                    / 9.0;
            }
        }
    }
}

int main()
{
    init_array();
    kernel_seidel_2d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2][N / 2]);

    return 0;
}