#include <stdio.h>
#include <stdlib.h>

#define N 120
#define TSTEPS 500

double A[N][N][N];
double B[N][N][N];

void init_array()
{
    int i, j, k;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                A[i][j][k] =
                B[i][j][k] =
                    (double)(i + j + (N - k)) * 10.0 / N;
            }
        }
    }
}

void kernel_heat_3d()
{
    int t, i, j, k;

    for (t = 1; t <= TSTEPS; t++)
    {
        /* -------------------------------------------------- */
        /* Region 0: A -> B stencil                           */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for collapse(3) map(to:A[0:N][0:N][0:N]) map(tofrom:B[0:N][0:N][0:N])
        for (i = 1; i < N - 1; i++)
        {
            for (j = 1; j < N - 1; j++)
            {
                for (k = 1; k < N - 1; k++)
                {
                    B[i][j][k] =
                          0.125 *
                          (A[i + 1][j][k]
                           - 2.0 * A[i][j][k]
                           + A[i - 1][j][k])

                        + 0.125 *
                          (A[i][j + 1][k]
                           - 2.0 * A[i][j][k]
                           + A[i][j - 1][k])

                        + 0.125 *
                          (A[i][j][k + 1]
                           - 2.0 * A[i][j][k]
                           + A[i][j][k - 1])

                        + A[i][j][k];
                }
            }
        }
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: B -> A stencil                           */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for collapse(3) map(to:B[0:N][0:N][0:N]) map(tofrom:A[0:N][0:N][0:N])
        for (i = 1; i < N - 1; i++)
        {
            for (j = 1; j < N - 1; j++)
            {
                for (k = 1; k < N - 1; k++)
                {
                    A[i][j][k] =
                          0.125 *
                          (B[i + 1][j][k]
                           - 2.0 * B[i][j][k]
                           + B[i - 1][j][k])

                        + 0.125 *
                          (B[i][j + 1][k]
                           - 2.0 * B[i][j][k]
                           + B[i][j - 1][k])

                        + 0.125 *
                          (B[i][j][k + 1]
                           - 2.0 * B[i][j][k]
                           + B[i][j][k - 1])

                        + B[i][j][k];
                }
            }
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_heat_3d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2][N / 2][N / 2]);

    return 0;
}