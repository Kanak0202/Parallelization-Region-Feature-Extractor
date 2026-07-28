#include <stdio.h>
#define N 1000000

int main()
{
    double A[N], B[N], C[N];
    int i;
    for (i = 0; i < N; i++) { A[i] = (double)(i+1); B[i] = (double)(i+2); }

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
        C[i] = A[i] / B[i];
    #pragma capc profitability_region end

    printf("%f\n", C[0]);
    return 0;
}