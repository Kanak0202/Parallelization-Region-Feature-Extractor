#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M 1900
#define N 2100

double A[N][M];
double s[M];
double q[N];
double p[M];
double r[N];

void init_array()
{
    for (int i = 0; i < M; i++)
        p[i] = (double)(i % M) / M;

    for (int i = 0; i < N; i++)
    {
        r[i] = (double)(i % N) / N;

        for (int j = 0; j < M; j++)
            A[i][j] = (double)(i * (j + 1) % N) / N;
    }
}

void kernel_bicg()
{
    int i, j;

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < M; i++)
        s[i] = 0.0;
#pragma capc profitability_region end

    for (i = 0; i < N; i++)
    {
        q[i] = 0.0;

#pragma capc profitability_region begin
#pragma omp parallel for private(j)
        for (j = 0; j < M; j++)
        {
            s[j] = s[j] + r[i] * A[i][j];

#pragma omp atomic update
            q[i] = q[i] + A[i][j] * p[j];
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_bicg();

    /* prevent complete dead-code elimination */
    printf("%f %f\n", s[0], q[0]);

    return 0;
}