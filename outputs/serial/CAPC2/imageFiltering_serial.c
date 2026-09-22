
#include <stdio.h>

#define N 512

int main()
{
    static int image[N][N];
    static int filtered[N][N];
    static int enhanced[N][N];
    static int output[N][N];

    int i, j;

    // Initialize image
    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            image[i][j] = (i * 11 + j * 13) % 256;
        }
    }
    #pragma capc profitability_region end

    // 3x3 average filter
    // Boundary pixels are copied independently.
    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (i == 0 || j == 0 ||
                i == N - 1 || j == N - 1)
            {
                filtered[i][j] = image[i][j];
            }
            else
            {
                filtered[i][j] =
                    (image[i-1][j-1] + image[i-1][j] +
                     image[i-1][j+1] + image[i][j-1] +
                     image[i][j] + image[i][j+1] +
                     image[i+1][j-1] + image[i+1][j] +
                     image[i+1][j+1]) / 9;
            }
        }
    }
    #pragma capc profitability_region end

    // Enhance image
    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (filtered[i][j] > 180)
                enhanced[i][j] = filtered[i][j] / 2;
            else if (filtered[i][j] > 100)
                enhanced[i][j] = filtered[i][j] * 2;
            else
                enhanced[i][j] = filtered[i][j] + 20;
        }
    }
    #pragma capc profitability_region end

    // Threshold classification
    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (enhanced[i][j] > 200)
                output[i][j] = 255;
            else if (enhanced[i][j] > 100)
                output[i][j] = 128;
            else
                output[i][j] = 0;
        }
    }
    #pragma capc profitability_region end

    printf("Output[%d][%d] = %d\n", N/2, N/2, output[N/2][N/2]);

    return 0;
}