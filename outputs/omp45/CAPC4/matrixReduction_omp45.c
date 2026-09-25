#include <stdio.h>

#define N 256

double A[N][N];
double B[N][N];
double C[N][N];

int main()
{
    int i, j, k;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = (double)((i + j) % 17) + 1.0;
            B[i][j] = (double)((i * 2 + j) % 13) + 1.0;
        }
    }

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for collapse(2) \
    map(to:A[0:N], B[0:N]) \
    map(from:C[0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            double sum = 0.0;

            for (k = 0; k < N; k++)
            {
                sum += A[i][k] * B[k][j];
            }

            C[i][j] = sum;
        }
    }
#pragma capc profitability_region end

    printf("%f %f\n", C[0][0], C[N - 1][N - 1]);

    return 0;
}