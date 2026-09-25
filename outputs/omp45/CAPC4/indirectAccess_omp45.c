#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
int Index[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)(i + 1);
        Index[i] = N - 1 - i;
    }

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for \
    map(to:A[0:N], Index[0:N]) \
    map(from:B[0:N])
    for (i = 0; i < N; i++)
    {
        int j = Index[i];

        B[i] = A[j] * 2.0 + A[j] * 0.5;
    }
#pragma capc profitability_region end

    printf("%f %f\n", B[0], B[N - 1]);

    return 0;
}