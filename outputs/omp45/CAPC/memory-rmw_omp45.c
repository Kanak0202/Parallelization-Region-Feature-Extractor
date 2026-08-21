#include <stdio.h>
#include <omp.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization

       A and B are write-only, so map(from:) is appropriate.
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i) \
    map(from:A[0:N], B[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(N - i);
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Read-modify-write A

       A must move H2D and then back D2H.
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i) \
    map(tofrom:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = 1.5 * A[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Read-modify-write B
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i) \
    map(tofrom:B[0:N])
    for (i = 0; i < N; i++)
    {
        B[i] = B[i] + 2.0;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Combine

       A and B are read-only.
       C is write-only.
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(i) \
    map(to:A[0:N], B[0:N]) \
    map(from:C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }
#pragma capc profitability_region end


    printf("A[0] = %f\n", A[0]);
    printf("B[N-1] = %f\n", B[N - 1]);
    printf("C[N-1] = %f\n", C[N - 1]);

    return 0;
}