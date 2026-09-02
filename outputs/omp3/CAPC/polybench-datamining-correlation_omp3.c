#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M 1200
#define N 1400

double data[N][M];
double corr[M][M];
double mean[M];
double stddev[M];

void init_array()
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            data[i][j] = (double)(i * j) / M + i;
}

void kernel_correlation()
{
    int i, j, k;
    double float_n = N;
    double eps = 0.1;

    /*
     * Calculate the mean of each column.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(j,i)
    for (j = 0; j < M; j++)
    {
        mean[j] = 0.0;

        for (i = 0; i < N; i++)
            mean[j] += data[i][j];

        mean[j] /= float_n;
    }
#pragma capc profitability_region end

    /*
     * Calculate the standard deviation of each column.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(j,i)
    for (j = 0; j < M; j++)
    {
        stddev[j] = 0.0;

        for (i = 0; i < N; i++)
            stddev[j] +=
                (data[i][j] - mean[j]) *
                (data[i][j] - mean[j]);

        stddev[j] /= float_n;
        stddev[j] = sqrt(stddev[j]);

        stddev[j] = stddev[j] <= eps ? 1.0 : stddev[j];
    }
#pragma capc profitability_region end

    /*
     * Center and reduce the column vectors.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
#pragma omp parallel for private(j)
        for (j = 0; j < M; j++)
        {
            data[i][j] -= mean[j];
            data[i][j] /= sqrt(float_n) * stddev[j];
        }
#pragma capc profitability_region end

    /*
     * Calculate the M x M correlation matrix.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(i,j,k)
    for (i = 0; i < M - 1; i++)
    {
        corr[i][i] = 1.0;

#pragma omp parallel for private(j,k)
        for (j = i + 1; j < M; j++)
        {
            corr[i][j] = 0.0;

            for (k = 0; k < N; k++)
                corr[i][j] += data[k][i] * data[k][j];

            corr[j][i] = corr[i][j];
        }
    }

    corr[M - 1][M - 1] = 1.0;
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_correlation();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", corr[0][0]);

    return 0;
}