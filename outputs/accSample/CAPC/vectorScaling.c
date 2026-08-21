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

    #pragma acc enter data copyin(A[0:N])

    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = 2.5 * A[i];
    }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:N])

    printf("A[0] = %f\n", A[0]);
    printf("A[N-1] = %f\n", A[N-1]);

    #pragma acc exit data delete(A[0:N])

    return 0;
}