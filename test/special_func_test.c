#include <stdio.h>
#include <math.h>
#define N 1000000

int main()
{
    double A[N], B[N];
    int i;
    for (i = 0; i < N; i++) A[i] = (double)(i+1);

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
        B[i] = sqrt(A[i]);
    #pragma capc profitability_region end

    printf("%f\n", B[0]);
    return 0;
}