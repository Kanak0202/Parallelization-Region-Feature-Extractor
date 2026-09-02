#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M 1000
#define N 1200

double A[M][M];
double B[M][N];

void init_array()
{
    double alpha = 1.5;

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < i; j++)
            A[i][j] = (double)((i + j) % M) / M;

        A[i][i] = 1.0;

        for (int j = 0; j < N; j++)
            B[i][j] = (double)((N + (i - j)) % N) / N;
    }
}

void kernel_trmm()
{
    int i, j, k;
    double alpha = 1.5;

    for (i = 0; i < M; i++)
    {
        #pragma capc profitability_region begin
        #pragma acc parallel loop copyin(A[0:M][0:M]) copy(B[0:M][0:N])
        for (j = 0; j < N; j++)
        {
            for (k = i + 1; k < M; k++)
                B[i][j] += A[k][i] * B[k][j];

            B[i][j] = alpha * B[i][j];
        }
        #pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_trmm();

    /* prevent complete dead-code elimination */
    printf("%f\n", B[0][0]);

    return 0;
}