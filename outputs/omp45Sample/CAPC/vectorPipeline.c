#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    #pragma omp target enter data map(alloc:A[0:N], B[0:N], C[0:N])

    /* =========================================================
       Region 1: Write-only initialization
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N], B[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(2 * i);
    }
    #pragma capc profitability_region end


    /* =========================================================
       Region 2: Read A,B -> Write C
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N], B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }
    #pragma capc profitability_region end


    /* =========================================================
       Region 3: Read A,C -> Write A
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = A[i] + 2.5 * C[i];
    }
    #pragma capc profitability_region end


    #pragma omp target update from(A[0:N], C[0:N])

    printf("A[N-1] = %f\n", A[N-1]);
    printf("C[N-1] = %f\n", C[N-1]);

    #pragma omp target exit data map(delete:A[0:N], B[0:N], C[0:N])

    return 0;
}