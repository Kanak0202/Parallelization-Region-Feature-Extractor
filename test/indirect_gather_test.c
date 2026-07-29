#include <stdio.h>
#define N 1000000

int main()
{
    double A[N], C[N];
    int idx[N];
    int i;
    for (i = 0; i < N; i++) { A[i] = (double)i; idx[i] = (N-1) - i; }

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
        C[i] = A[idx[i]];
    #pragma capc profitability_region end

    printf("%f\n", C[0]);
    return 0;
}