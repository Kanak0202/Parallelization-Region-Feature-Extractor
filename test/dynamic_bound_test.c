// test/dynamic_bound_test.c
#include <stdio.h>

void compute(double *A, double *B, double *C, int n)
{
    #pragma capc profitability_region begin
    for (int i = 0; i < n; i++)
    {
        C[i] = A[i] + B[i];
    }
    #pragma capc profitability_region end
}

int main()
{
    int n = 500000;
    double A[500000], B[500000], C[500000];
    compute(A, B, C, n);
    printf("C[0] = %f\n", C[0]);
    return 0;
}