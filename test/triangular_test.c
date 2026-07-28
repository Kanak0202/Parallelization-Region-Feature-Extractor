#include <stdio.h>
#define N 5000

int main()
{
    double A[N][N];
    int i, j;
    for (i = 0; i < N; i++) for (j = 0; j < N; j++) A[i][j] = 0.0;

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
        for (j = 0; j <= i; j++)
            A[i][j] = A[i][j] + 1.0;
    #pragma capc profitability_region end

    printf("%f\n", A[N-1][0]);
    return 0;
}