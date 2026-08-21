#include <stdio.h>

#define N 10000000

double A[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
    }

    #pragma omp target enter data map(to:A[0:N])

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = 2.5 * A[i];
    }
    #pragma capc profitability_region end

    #pragma omp target update from(A[0:N])

    printf("A[0] = %f\n", A[0]);
    printf("A[N-1] = %f\n", A[N-1]);

    #pragma omp target exit data map(delete:A[0:N])

    return 0;
}