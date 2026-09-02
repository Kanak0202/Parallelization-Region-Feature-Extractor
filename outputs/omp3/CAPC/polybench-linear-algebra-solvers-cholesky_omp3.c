#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2000

double A[N][N];
double B[N][N];

void init_array()
{
    int i, j, r, s, t;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j <= i; j++)
            A[i][j] = (double)(-j % N) / N + 1.0;

        for (j = i + 1; j < N; j++)
            A[i][j] = 0.0;

        A[i][i] = 1.0;
    }

    /* Make A positive semi-definite. */
    for (r = 0; r < N; r++)
        for (s = 0; s < N; s++)
            B[r][s] = 0.0;

    for (t = 0; t < N; t++)
        for (r = 0; r < N; r++)
            for (s = 0; s < N; s++)
                B[r][s] += A[r][t] * A[s][t];

    for (r = 0; r < N; r++)
        for (s = 0; s < N; s++)
            A[r][s] = B[r][s];
}

void kernel_cholesky()
{
    int i, j, k;

    for (i = 0; i < N; i++)
    {
        /* j < i */
        for (j = 0; j < i; j++)
        {
#pragma capc profitability_region begin
#pragma omp parallel for private(k)
            for (k = 0; k < j; k++)
            {
#pragma omp atomic update
                A[i][j] -= A[i][k] * A[j][k];
            }
#pragma capc profitability_region end

            A[i][j] /= A[j][j];
        }

        /* i == j */
#pragma capc profitability_region begin
#pragma omp parallel for private(k)
        for (k = 0; k < i; k++)
        {
#pragma omp atomic update
            A[i][i] -= A[i][k] * A[i][k];
        }
#pragma capc profitability_region end

        A[i][i] = sqrt(A[i][i]);
    }
}

int main()
{
    init_array();
    kernel_cholesky();

    /* prevent complete dead-code elimination */
    printf("%f\n", A[N - 1][N - 1]);

    return 0;
}