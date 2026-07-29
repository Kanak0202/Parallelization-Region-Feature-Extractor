#include <stdio.h>
#define N 1000000

int main()
{
    double A[N], B[N];
    double sum = 0.0, prod = 1.0;
    int i;
    for (i = 0; i < N; i++) { A[i] = (double)(i+1); B[i] = (double)(i+2); }

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++) {
        sum += A[i];
        prod *= B[i];
    }
    #pragma capc profitability_region end

    printf("%f %f\n", sum, prod);
    return 0;
}