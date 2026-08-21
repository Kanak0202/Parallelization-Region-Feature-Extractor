#include <stdio.h>

#define N 10000000

double A[N];
double C[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = i;
        C[i] = 0.0;
    }

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(to:A[0:N]) map(from:C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = 3.0 * A[i];
    }
    #pragma capc profitability_region end

    printf("C[N-1] = %f\n", C[N-1]);

    return 0;
}