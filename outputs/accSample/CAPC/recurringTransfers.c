#include <stdio.h>

#define N 1000000
#define ITER 20

double A[N];

int main()
{
    int i, t;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
    }

    #pragma acc enter data create(A[0:N])

    for (t = 0; t < ITER; t++)
    {
        #pragma acc update device(A[0:N])

        #pragma capc profitability_region begin
        #pragma acc parallel loop present(A[0:N])
        for (i = 0; i < N; i++)
        {
            A[i] *= 2.0;
        }
        #pragma capc profitability_region end

        #pragma acc update self(A[0:N])
    }

    #pragma acc exit data delete(A[0:N])

    printf("A[0] = %f\n", A[0]);
    printf("A[N-1] = %f\n", A[N-1]);

    return 0;
}