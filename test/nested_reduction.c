#include <stdio.h>

#define N 100
#define M 200

int main()
{
    double A[N][M];
    double sum = 0.0;

    // Initialize array
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            A[i][j] = i + j;
        }
    }

#pragma capc profitability_region begin

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < M; j++)
        {
            sum += A[i][j];
        }
    }

#pragma capc profitability_region end

    printf("Sum = %f\n", sum);

    return 0;
}