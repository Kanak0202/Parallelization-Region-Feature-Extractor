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
    for (i = 0; i < N; i++)
    {
        A[i] = B[index_array[i]];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Gather + streaming read
       ============================================================ */

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[index_array[i]];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Streaming update
       ============================================================ */

#pragma capc profitability_region begin
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