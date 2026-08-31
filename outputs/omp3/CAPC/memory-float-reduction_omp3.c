#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;
    double sum = 0.0;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(N - i);
        C[i] = 0.5 * (double)i;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Floating-point reduction
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp parallel for reduction(+:sum) private(i)
    for (i = 0; i < N; i++)
    {
        sum += A[i] + B[i] * C[i];
    }
#pragma capc profitability_region end

    printf("sum = %f\n", sum);

    return 0;
}