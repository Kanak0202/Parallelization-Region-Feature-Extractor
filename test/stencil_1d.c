#include <stdio.h>
#define N 1000000

int main()
{
    double A[N], B[N];
    int i;
    for (i = 0; i < N; i++) A[i] = (double)i;

    #pragma capc profitability_region begin
    for (i = 1; i < N - 1; i++)
        B[i] = A[i-1] + A[i] + A[i+1];
    #pragma capc profitability_region end

    printf("%f\n", B[1]);
    return 0;
}