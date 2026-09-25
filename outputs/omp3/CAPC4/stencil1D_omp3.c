#include <stdio.h>

#define N 10000000

double A[N];
double B[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
        A[i] = (double)(i % 100);

#pragma capc profitability_region begin
#pragma omp parallel for
    for (i = 1; i < N - 1; i++)
    {
        B[i] = 0.25 * A[i - 1]
             + 0.50 * A[i]
             + 0.25 * A[i + 1];
    }
#pragma capc profitability_region end

    printf("%f %f\n", B[1], B[N - 2]);

    return 0;
}