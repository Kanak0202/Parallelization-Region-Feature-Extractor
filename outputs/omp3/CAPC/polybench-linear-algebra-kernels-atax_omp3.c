#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M 1900
#define N 2100

double A[M][N];
double x[N];
double y[N];
double tmp[M];

void init_array()
{
    double fn = (double)N;

    for (int i = 0; i < N; i++)
        x[i] = 1.0 + (i / fn);

    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            A[i][j] = (double)((i + j) % N) / (5 * M);
}

void kernel_atax()
{
    int i, j;

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
        y[i] = 0.0;
#pragma capc profitability_region end

    for (i = 0; i < M; i++)
    {
        tmp[i] = 0.0;

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (j = 0; j < N; j++)
        {
#pragma omp atomic update
            tmp[i] = tmp[i] + A[i][j] * x[j];
        }
#pragma capc profitability_region end

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (j = 0; j < N; j++)
            y[j] = y[j] + A[i][j] * tmp[i];
#pragma capc profitability_region end
    }

}

int main()
{
    init_array();
    kernel_atax();

    /* prevent complete dead-code elimination */
    printf("%f\n", y[0]);

    return 0;
}