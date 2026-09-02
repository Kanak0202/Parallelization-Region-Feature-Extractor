#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2000

double A[N][N];
double B[N][N];

double b[N];
double x[N];
double y[N];

void init_array()
{
    int i, j, r, s, t;
    double fn = (double)N;

    for (i = 0; i < N; i++)
    {
        x[i] = 0.0;
        y[i] = 0.0;
        b[i] = (i + 1) / fn / 2.0 + 4.0;
    }

    for (i = 0; i < N; i++)
    {
        for (j = 0; j <= i; j++)
            A[i][j] = (double)(-j % N) / N + 1.0;

        for (j = i + 1; j < N; j++)
            A[i][j] = 0.0;

        A[i][i] = 1.0;
    }

    /* Make the matrix positive semi-definite. */
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

void kernel_ludcmp()
{
    int i, j, k;
    double w;

    /* -------------------------------------------------- */
    /* LU decomposition                                   */
    /* -------------------------------------------------- */

    for (i = 0; i < N; i++)
    {
        /* Lower triangular part */
        for (j = 0; j < i; j++)
        {
            w = A[i][j];

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N][0:N]) copy(w)
            for (k = 0; k < j; k++)
            {
#pragma acc atomic update
                w -= A[i][k] * A[k][j];
            }
#pragma capc profitability_region end

            A[i][j] = w / A[j][j];
        }

        /* Upper triangular part */
#pragma capc profitability_region begin
#pragma acc parallel loop private(k,w) copy(A[0:N][0:N])
        for (j = i; j < N; j++)
        {
            w = A[i][j];

            for (k = 0; k < i; k++)
                w -= A[i][k] * A[k][j];

            A[i][j] = w;
        }
#pragma capc profitability_region end
    }

    /* -------------------------------------------------- */
    /* Forward substitution                               */
    /* -------------------------------------------------- */

    for (i = 0; i < N; i++)
    {
        w = b[i];

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N][0:N],y[0:N]) copy(w)
        for (j = 0; j < i; j++)
        {
#pragma acc atomic update
            w -= A[i][j] * y[j];
        }
#pragma capc profitability_region end

        y[i] = w;
    }

    /* -------------------------------------------------- */
    /* Backward substitution                              */
    /* -------------------------------------------------- */

    for (i = N - 1; i >= 0; i--)
    {
        w = y[i];

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N][0:N],x[0:N]) copy(w)
        for (j = i + 1; j < N; j++)
        {
#pragma acc atomic update
            w -= A[i][j] * x[j];
        }
#pragma capc profitability_region end

        x[i] = w / A[i][i];
    }
}

int main()
{
    init_array();
    kernel_ludcmp();

    /* prevent complete dead-code elimination */
    printf("%f\n", x[0]);

    return 0;
}