#include <stdio.h>

#define N 1000000
#define ITER 20

double A[N];

int main()
{
    int i, t;

    for (i = 0; i < N; i++)
    {
        A[i] = i;
    }

    #pragma omp target enter data map(alloc:A[0:N])

    for (t = 0; t < ITER; t++)
    {
        #pragma omp target update to(A[0:N])

        #pragma capc profitability_region begin
        #pragma omp target teams distribute parallel for map(alloc:A[0:N])
        for (i = 0; i < N; i++)
        {
            A[i] *= 2.0;
        }
        #pragma capc profitability_region end

        #pragma omp target update from(A[0:N])
    }

    #pragma omp target exit data map(delete:A[0:N])

    printf("A[0] = %f\n", A[0]);

    return 0;
}