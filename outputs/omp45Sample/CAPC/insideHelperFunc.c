#include <stdio.h>

#define N 10000000

double A[N];

void initialize()
{
    int i;

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(from:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = i * 0.25;
    }
    #pragma capc profitability_region end
}

int main()
{
    initialize();

    printf("A[N-1] = %f\n", A[N-1]);

    return 0;
}