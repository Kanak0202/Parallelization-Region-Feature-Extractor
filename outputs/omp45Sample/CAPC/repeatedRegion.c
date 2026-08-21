#include <stdio.h>

#define N 10000000
#define ITER 100

double A[N];

int main()
{
    int i, t;

    for (i = 0; i < N; i++)
    {
        A[i] = i;
    }

    #pragma omp target enter data map(to:A[0:N])

    for (t = 0; t < ITER; t++)
    {
        #pragma capc profitability_region begin
        #pragma omp target teams distribute parallel for map(alloc:A[0:N])
        for (i = 0; i < N; i++)
        {
            A[i] = A[i] + 1.0;
        }
        #pragma capc profitability_region end
    }

    #pragma omp target update from(A[0:N])

    printf("A[N-1] = %f\n", A[N-1]);

    #pragma omp target exit data map(delete:A[0:N])

    return 0;
}