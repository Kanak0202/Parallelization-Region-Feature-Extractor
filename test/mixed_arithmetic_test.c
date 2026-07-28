// test/mixed_arithmetic_test.c
#include <stdio.h>
#define N 1000000

int main()
{
    int B[N], C[N];
    int A[N], D[N], E[N], F[N];
    double Y[N], Z[N];
    double X[N], W[N], V[N], U[N];
    int i;

    for (i = 0; i < N; i++)
    {
        B[i] = i + 1;
        C[i] = (i % 50) + 1;      // avoid div-by-zero
        Y[i] = (double)(i + 1);
        Z[i] = (double)((i % 50) + 1);
    }

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        A[i] = B[i] + C[i];   // int add
        D[i] = B[i] - C[i];   // int sub
        E[i] = B[i] * C[i];   // int multiply
        F[i] = B[i] / C[i];   // int division
 
        // X[i] = Y[i] + Z[i];   // float add
        // W[i] = Y[i] - Z[i];   // float sub
        // V[i] = Y[i] * Z[i];   // float multiply
        // U[i] = Y[i] / Z[i];   // float division
    }
    #pragma capc profitability_region end

    printf("%d %f\n", F[0], U[0]);
    return 0;
}