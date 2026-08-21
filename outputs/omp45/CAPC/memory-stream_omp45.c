#include <stdio.h>
#include <omp.h>

#define N 10000000

double A[N];
double B[N];
double C[N];
double D[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i)
    for (i = 0; i < N; i++)
    {
        B[i] = (double)i;
        C[i] = (double)(N - i);
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Copy
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i)
    for (i = 0; i < N; i++)
    {
        A[i] = B[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Scale
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i)
    for (i = 0; i < N; i++)
    {
        D[i] = 2.5 * C[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Triad
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i)
    for (i = 0; i < N; i++)
    {
        A[i] = B[i] + 3.0 * D[i];
    }
#pragma capc profitability_region end


    printf("A[0] = %f\n", A[0]);
    printf("A[N-1] = %f\n", A[N - 1]);
    printf("D[0] = %f\n", D[0]);

    return 0;
}