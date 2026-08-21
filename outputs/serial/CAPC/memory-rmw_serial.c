#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(N - i);
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Read-modify-write A
       ============================================================ */

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        A[i] = 1.5 * A[i];
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 3: Read-modify-write B
       ============================================================ */

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        B[i] = B[i] + 2.0;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 4: Combine
       ============================================================ */

#pragma capc profitability_region begin
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