#include <stdio.h>
#include <stdlib.h>

#define M 1000
#define N 1200

double C[M][N];
double A[M][M];
double B[M][N];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
        {
            C[i][j] = (double)((i + j) % 100) / M;
            B[i][j] = (double)((N + i - j) % 100) / M;
        }

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j <= i; j++)
            A[i][j] = (double)((i + j) % 100) / M;

        for (int j = i + 1; j < M; j++)
            A[i][j] = -999.0;
    }
}

void kernel_symm()
{
    int i, j, k;
    double alpha = 1.5;
    double beta = 1.2;
    double temp2;

    for (i = 0; i < M; i++)
    {
        #pragma capc profitability_region begin
#pragma omp parallel for private(j,k,temp2)
        for (j = 0; j < N; j++)
        {
            temp2 = 0.0;

            for (k = 0; k < i; k++)
            {
                C[k][j] += alpha * B[i][j] * A[i][k];
                temp2 += B[k][j] * A[i][k];
            }

            C[i][j] = beta * C[i][j]
                    + alpha * B[i][j] * A[i][i]
                    + alpha * temp2;
        }
        #pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_symm();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}