#include <stdio.h>
#include <stdlib.h>

#define N 1000
#define M 1200

double C[N][N];
double A[N][M];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            A[i][j] = (double)((i * j + 1) % N) / N;

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            C[i][j] = (double)((i * j + 2) % M) / M;
}

void kernel_syrk()
{
    int i, j, k;

    double alpha = 1.5;
    double beta = 1.2;

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N][0:M]) copy(C[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j <= i; j++)
            C[i][j] *= beta;

        for (k = 0; k < M; k++)
        {
            for (j = 0; j <= i; j++)
                C[i][j] += alpha * A[i][k] * A[j][k];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_syrk();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}