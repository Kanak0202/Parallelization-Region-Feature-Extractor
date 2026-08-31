#include <stdio.h>

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
       Write-only B and C
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(from:B[0:N],C[0:N])
    for (i = 0; i < N; i++)
    {
        B[i] = (double)i;
        C[i] = (double)(N - i);
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Copy
       One load + one store
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:B[0:N]) map(from:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = B[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Scale
       One load + one store
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:C[0:N]) map(from:D[0:N])
    for (i = 0; i < N; i++)
    {
        D[i] = 2.5 * C[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Triad
       Two loads + one store
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:B[0:N],D[0:N]) map(from:A[0:N])
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