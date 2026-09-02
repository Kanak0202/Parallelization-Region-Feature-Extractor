#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2000

double A[N][N];

double x1[N];
double x2[N];
double y_1[N];
double y_2[N];

void init_array()
{
    for (int i = 0; i < N; i++)
    {
        x1[i] = (double)(i % N) / N;
        x2[i] = (double)((i + 1) % N) / N;
        y_1[i] = (double)((i + 3) % N) / N;
        y_2[i] = (double)((i + 4) % N) / N;

        for (int j = 0; j < N; j++)
            A[i][j] = (double)(i * j % N) / N;
    }
}

void kernel_mvt()
{
    int i, j;

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(j) map(to:A[0:N][0:N],y_1[0:N]) map(tofrom:x1[0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            x1[i] = x1[i] + A[i][j] * y_1[j];
    }
#pragma capc profitability_region end

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for private(j) map(to:A[0:N][0:N],y_2[0:N]) map(tofrom:x2[0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            x2[i] = x2[i] + A[j][i] * y_2[j];
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_mvt();

    /* prevent complete dead-code elimination */
    printf("%f %f\n", x1[0], x2[0]);

    return 0;
}