#include <stdio.h>

#define N 5000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = 0.0;
        C[i] = 0.0;
    }

    #pragma acc enter data copyin(A[0:N]) create(B[0:N], C[0:N])

    /* Region 1: pure copy */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], B[0:N])
    for (i = 0; i < N; i++)
    {
        B[i] = A[i];
    }
    #pragma capc profitability_region end

    /* Region 2: scale */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = 2.0 * B[i];
    }
    #pragma capc profitability_region end

    /* Region 3: add */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = B[i] + C[i];
    }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:N])

    printf("A[N-1] = %f\n", A[N-1]);

    #pragma acc exit data delete(A[0:N], B[0:N], C[0:N])

    return 0;
}