#include <stdio.h>

#define N 1

int main()
{
    static int assets[N][N];
    static int risk[N][N];
    static int normalized[N][N];
    static int category[N][N];

    int i, j;

    // Initialize asset data
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(from: assets[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            assets[i][j] = (i * 19 + j * 7) % 1000;
        }
    }
    #pragma capc profitability_region end

    // Calculate risk scores
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(to: assets[0:N][0:N]) map(from: risk[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            int value = assets[i][j];

            if (value > 700)
                risk[i][j] = value * 3 + 100;
            else if (value > 300)
                risk[i][j] = value * 2 + 50;
            else
                risk[i][j] = value + 20;
        }
    }
    #pragma capc profitability_region end

    // Normalize risk scores
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(to: risk[0:N][0:N]) map(from: normalized[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (risk[i][j] > 2000)
                normalized[i][j] = risk[i][j] / 5;
            else if (risk[i][j] > 1000)
                normalized[i][j] = risk[i][j] / 3;
            else
                normalized[i][j] = risk[i][j] / 2;
        }
    }
    #pragma capc profitability_region end

    // Risk classification
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(to: normalized[0:N][0:N]) map(from: category[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (normalized[i][j] > 500)
                category[i][j] = 3;
            else if (normalized[i][j] > 250)
                category[i][j] = 2;
            else if (normalized[i][j] > 100)
                category[i][j] = 1;
            else
                category[i][j] = 0;
        }
    }
    #pragma capc profitability_region end

    // Final independent adjustment
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(to: normalized[0:N][0:N], category[0:N][0:N]) map(from: risk[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            risk[i][j] =
                normalized[i][j] + category[i][j] * 10;
        }
    }
    #pragma capc profitability_region end

    printf("Risk[%d][%d] = %d\n", N, N, risk[N][N]);
    printf("Category[%d][%d] = %d\n", N, N, category[N][N]);

    return 0;
}