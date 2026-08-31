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
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
    {
        A[i] = (double)(i + 1);
        B[i] = (double)(N - i) + 1.0;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Division + branch
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
    {
        if (B[i] > A[i])
        {
            C[i] = A[i] / B[i];
        }
        else
        {
            C[i] = B[i] / A[i];
        }
    }
#pragma capc profitability_region end

    printf("C[0] = %f\n", C[0]);
    printf("C[N-1] = %f\n", C[N - 1]);

    return 0;
}