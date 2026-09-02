#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2000

double L[N][N];
double x[N];
double b[N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        x[i] = -999.0;
        b[i] = (double)i;

        for (j = 0; j <= i; j++)
            L[i][j] = (double)(i + N - j + 1) * 2.0 / N;
    }
}

void kernel_trisolv()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        x[i] = b[i];

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (j = 0; j < i; j++)
        {
#pragma omp atomic update
            x[i] -= L[i][j] * x[j];
        }
#pragma capc profitability_region end

        x[i] = x[i] / L[i][i];
    }
}

int main()
{
    init_array();
    kernel_trisolv();

    /* prevent complete dead-code elimination */
    printf("%f\n", x[N - 1]);

    return 0;
}