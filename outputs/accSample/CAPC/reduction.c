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

    #pragma acc enter data copyin(A[0:N])

    #pragma capc profitability_region begin
    #pragma acc parallel loop reduction(+:sum) present(A[0:N])
    for (i = 0; i < N; i++)
    {
        sum += A[i];
    }
    #pragma capc profitability_region end

    printf("sum = %f\n", sum);

    #pragma acc exit data delete(A[0:N])

    return 0;
}