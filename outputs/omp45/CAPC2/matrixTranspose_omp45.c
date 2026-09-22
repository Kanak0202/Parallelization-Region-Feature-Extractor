
#include <stdio.h>

#define N 512

int main()
{
    static int A[N][N];
    static int B[N][N];
    static int C[N][N];
    static int D[N][N];

    int i, j;

    // Initialize matrix
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) \
        map(from: A[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = (i * 3 + j * 7) % 1000;
        }
    }
    #pragma capc profitability_region end

    // Transpose matrix
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) \
        map(to: A[0:N][0:N]) \
        map(from: B[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            B[j][i] = A[i][j];
        }
    }
    #pragma capc profitability_region end

    // Conditional normalization
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) \
        map(to: B[0:N][0:N]) \
        map(from: C[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            int value = B[i][j];

            if (value > 750)
                C[i][j] = value / 5;
            else if (value > 300)
                C[i][j] = value / 3;
            else
                C[i][j] = value / 2;
        }
    }
    #pragma capc profitability_region end

    // Independent matrix transformation
    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) \
        map(to: C[0:N][0:N]) \
        map(from: D[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if ((i + j) % 2 == 0)
                D[i][j] = C[i][j] + 10;
            else
                D[i][j] = C[i][j] - 10;
        }
    }
    #pragma capc profitability_region end

    printf("D[0][0] = %d\n", D[0][0]);

    return 0;
}