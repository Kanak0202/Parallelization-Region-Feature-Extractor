
#include <stdio.h>

#define N 1000000

int main()
{
    static int a[N], b[N], normalized[N], category[N];
    int i;

    // Initialize arrays
    #pragma capc profitability_region begin
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        a[i] = (i * 17 + 13) % 1000;
        b[i] = (i * 23 + 7) % 1000;
    }
    #pragma capc profitability_region end

    // Integer division and conditional processing
    #pragma capc profitability_region begin
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        int value = a[i] + b[i];

        if (value > 1200)
            normalized[i] = value / 4;
        else if (value > 600)
            normalized[i] = value / 3;
        else
            normalized[i] = value / 2;
    }
    #pragma capc profitability_region end

    // Classification
    #pragma capc profitability_region begin
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        if (normalized[i] > 300)
            category[i] = 3;
        else if (normalized[i] > 150)
            category[i] = 2;
        else if (normalized[i] > 50)
            category[i] = 1;
        else
            category[i] = 0;
    }
    #pragma capc profitability_region end

    // Independent transformation
    #pragma capc profitability_region begin
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        b[i] = (normalized[i] * 7 + category[i]) / 2;
    }
    #pragma capc profitability_region end

    printf("Normalized[%d] = %d\n", N, normalized[N]);
    printf("Category[%d] = %d\n", N, category[N]);

    return 0;
}