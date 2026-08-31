#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int index_array[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(from:B[0:N],index_array[0:N])
    for (i = 0; i < N; i++)
    {
        B[i] = (double)(i + 1);
        index_array[i] = (i * 17) % N;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Indirect gather
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:B[0:N],index_array[0:N]) map(from:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = B[index_array[i]];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Gather + streaming read
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:A[0:N],B[0:N],index_array[0:N]) map(from:C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[index_array[i]];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Streaming update
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:C[0:N]) map(from:B[0:N])
    for (i = 0; i < N; i++)
    {
        B[i] = C[i] + 1.0;
    }
#pragma capc profitability_region end


    printf("A[0] = %f\n", A[0]);
    printf("C[0] = %f\n", C[0]);
    printf("B[N-1] = %f\n", B[N - 1]);

    return 0;
}