#include <stdio.h>

#define N 10000000

double A[N];

int main()
{
    int i;
    double sum = 0.0;

    #pragma omp target enter data map(alloc:A[0:N])


    /* =========================================================
       Region 1: Write-only
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i * 0.5;
    }
    #pragma capc profitability_region end


    /* =========================================================
       Region 2: Read-write
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = A[i] * A[i] + 2.0 * A[i];
    }
    #pragma capc profitability_region end


    /* =========================================================
       Region 3: Reduction
       ========================================================= */
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for reduction(+:sum) map(alloc:A[0:N])
    for (i = 0; i < N; i++)
    {
        sum += A[i];
    }
    #pragma capc profitability_region end


    #pragma omp target update from(A[0:N])

    printf("A[N-1] = %f\n", A[N-1]);
    printf("sum = %f\n", sum);

    #pragma omp target exit data map(delete:A[0:N])

    return 0;
}