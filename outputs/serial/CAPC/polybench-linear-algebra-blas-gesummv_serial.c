#include <stdio.h>
#include <stdlib.h>

#define N 1300

double A[N][N];
double B[N][N];
double tmp[N];
double x[N];
double y[N];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < N; i++)
    {
        x[i] = (double)(i % N) / N;

        for (int j = 0; j < N; j++)
        {
            A[i][j] = (double)((i * j + 1) % N) / N;
            B[i][j] = (double)((i * j + 2) % N) / N;
        }
    }
}

void kernel_gesummv()
{
    int i, j;

    double alpha = 1.5;
    double beta = 1.2;

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        tmp[i] = 0.0;
        y[i] = 0.0;

        for (j = 0; j < N; j++)
        {
            tmp[i] = A[i][j] * x[j] + tmp[i];
            y[i] = B[i][j] * x[j] + y[i];
        }

        y[i] = alpha * tmp[i] + beta * y[i];
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_gesummv();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", y[0]);

    return 0;
}
