#include <stdio.h>

#define N 10000000

double A[N];

int main()
{
    int i;
    double sum = 0.0;

    for (i = 0; i < N; i++)
    {
        A[i] = 1.0;
    }

    #pragma omp target enter data map(to:A[0:N])

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for reduction(+:sum) map(alloc:A[0:N])
    for (i = 0; i < N; i++)
    {
        sum += A[i];
    }
    #pragma capc profitability_region end

    printf("sum = %f\n", sum);

    #pragma omp target exit data map(delete:A[0:N])

    return 0;
}