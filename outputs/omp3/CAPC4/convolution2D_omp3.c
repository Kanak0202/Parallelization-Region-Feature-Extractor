#include <stdio.h>

#define N 2048

double A[N][N];
double B[N][N];

int main()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = (double)((i + j) % 100);
        }
    }

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
    for (i = 1; i < N - 1; i++)
    {
        for (j = 1; j < N - 1; j++)
        {
            B[i][j] =
                  A[i - 1][j - 1] * 0.0625
                + A[i - 1][j]     * 0.125
                + A[i - 1][j + 1] * 0.0625
                + A[i][j - 1]     * 0.125
                + A[i][j]         * 0.25
                + A[i][j + 1]     * 0.125
                + A[i + 1][j - 1] * 0.0625
                + A[i + 1][j]     * 0.125
                + A[i + 1][j + 1] * 0.0625;
        }
    }
#pragma capc profitability_region end

    printf("%f %f\n", B[1][1], B[N - 2][N - 2]);

    return 0;
}