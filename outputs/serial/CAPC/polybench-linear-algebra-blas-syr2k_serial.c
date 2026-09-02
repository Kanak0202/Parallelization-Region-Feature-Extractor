#include <stdio.h>
#include <stdlib.h>

#define M 1000
#define N 1200

double C[N][N];
double A[N][M];
double B[N][M];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
        {
            A[i][j] = (double)((i * j + 1) % N) / N;
            B[i][j] = (double)((i * j + 2) % M) / M;
        }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C[i][j] = (double)((i * j + 3) % N) / M;
}

void kernel_syr2k()
{
    int i, j, k;

    double alpha = 1.5;
    double beta = 1.2;

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        for (j = 0; j <= i; j++)
            C[i][j] *= beta;

        for (k = 0; k < M; k++)
            for (j = 0; j <= i; j++)
            {
                C[i][j] +=
                    A[j][k] * alpha * B[i][k]
                    + B[j][k] * alpha * A[i][k];
            }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_syr2k();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}