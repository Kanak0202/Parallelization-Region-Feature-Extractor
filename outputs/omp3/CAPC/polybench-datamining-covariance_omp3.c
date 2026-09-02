#include <stdio.h>
#include <stdlib.h>

#define M 1200
#define N 1400

double data[N][M];
double cov[M][M];
double mean[M];

void init_array()
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            data[i][j] = (double)(i * j) / M;
}

void kernel_covariance()
{
    int i, j, k;
    double float_n = N;

    /*
     * Calculate the mean of each column.
     *
     * The j iterations are independent.
     * The inner i loop is a reduction into mean[j].
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
     * Subtract the mean from every element.
     *
     * Every (i,j) iteration modifies a different
     * data[i][j], so the iterations are independent.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
#pragma omp parallel for private(j)
        for (j = 0; j < M; j++)
            data[i][j] -= mean[j];
#pragma capc profitability_region end

    /*
     * Calculate the covariance matrix.
     *
     * Each (i,j) pair calculates one covariance value.
     * The k loop is a reduction into cov[i][j].
     *
     * For every pair, cov[i][j] and cov[j][i] are
     * written together, and different (i,j) pairs do
     * not write the same matrix elements.
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(i,j,k)
    for (i = 0; i < M; i++)
    {
#pragma omp parallel for private(j,k)
        for (j = i; j < M; j++)
        {
            cov[i][j] = 0.0;

            for (k = 0; k < N; k++)
                cov[i][j] += data[k][i] * data[k][j];

            cov[i][j] /= (float_n - 1.0);
            cov[j][i] = cov[i][j];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_covariance();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", cov[0][0]);

    return 0;
}