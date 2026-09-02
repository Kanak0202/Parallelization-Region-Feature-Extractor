#include <stdio.h>
#include <stdlib.h>

#define N 1300
#define TSTEPS 500

double A[N][N];
double B[N][N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = ((double)i * (j + 2) + 2.0) / N;
            B[i][j] = ((double)i * (j + 3) + 3.0) / N;
        }
    }
}

void kernel_jacobi_2d()
{
    int t, i, j;

    for (t = 0; t < TSTEPS; t++)
    {
        /* --------------------------------------------- */
        /* Region 0: A -> B 5-point stencil              */
        /* --------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (i = 1; i < N - 1; i++)
        {
#pragma omp parallel for private(j)
            for (j = 1; j < N - 1; j++)
            {
                B[i][j] =
                    0.2 *
                    (A[i][j]
                     + A[i][j - 1]
                     + A[i][j + 1]
                     + A[i + 1][j]
                     + A[i - 1][j]);
            }
        }
#pragma capc profitability_region end

        /* --------------------------------------------- */
        /* Region 1: B -> A 5-point stencil              */
        /* --------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (i = 1; i < N - 1; i++)
        {
#pragma omp parallel for private(j)
            for (j = 1; j < N - 1; j++)
            {
                A[i][j] =
                    0.2 *
                    (B[i][j]
                     + B[i][j - 1]
                     + B[i][j + 1]
                     + B[i + 1][j]
                     + B[i - 1][j]);
            }
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_jacobi_2d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2][N / 2]);

    return 0;
}