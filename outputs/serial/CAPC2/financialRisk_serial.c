
#include <stdio.h>

#define N 512

int main()
{
    static int assets[N][N];
    static int risk[N][N];
    static int normalized[N][N];
    static int category[N][N];

    int i, j;

    // Initialize asset data
    #pragma capc profitability_region begin
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
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            risk[i][j] = normalized[i][j] + category[i][j] * 10;
        }
    }
    #pragma capc profitability_region end

    printf("Risk[0][0] = %d\n", risk[0][0]);
    printf("Category[0][0] = %d\n", category[0][0]);

    return 0;
}