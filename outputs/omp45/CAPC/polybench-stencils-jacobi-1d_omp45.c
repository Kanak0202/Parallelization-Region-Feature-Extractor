#include <stdio.h>
#include <stdlib.h>

#define N 2000
#define TSTEPS 500

double A[N];
double B[N];

void init_array()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = ((double)i + 2.0) / N;
        B[i] = ((double)i + 3.0) / N;
    }
}

void kernel_jacobi_1d()
{
    int t, i;

    for (t = 0; t < TSTEPS; t++)
    {
        /* --------------------------------------------- */
        /* Region 0: A -> B stencil                      */
        /* --------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:A[0:N]) map(tofrom:B[0:N])
        for (i = 1; i < N - 1; i++)
        {
            B[i] = 0.33333 *
                   (A[i - 1] + A[i] + A[i + 1]);
        }
#pragma capc profitability_region end

        /* --------------------------------------------- */
        /* Region 1: B -> A stencil                      */
        /* --------------------------------------------- */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:B[0:N]) map(tofrom:A[0:N])
        for (i = 1; i < N - 1; i++)
        {
            A[i] = 0.33333 *
                   (B[i - 1] + B[i] + B[i + 1]);
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_jacobi_1d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2]);

    return 0;
}