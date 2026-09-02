#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define M 1000
#define N 1200

double A[M][N];
double R[N][N];
double Q[M][N];

void init_array()
{
    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j < N; j++)
        {
            A[i][j] =
                ((double)((i * j) % M) / M) * 100.0 + 10.0;

            Q[i][j] = 0.0;
        }
    }

    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            R[i][j] = 0.0;
}

void kernel_gramschmidt()
{
    int i, j, k;
    double nrm;


    for (k = 0; k < N; k++)
    {
        nrm = 0.0;
        #pragma capc profitability_region begin
        for (i = 0; i < M; i++)
            nrm += A[i][k] * A[i][k];
        #pragma capc profitability_region end

        R[k][k] = sqrt(nrm);
        #pragma capc profitability_region begin
        for (i = 0; i < M; i++)
            Q[i][k] = A[i][k] / R[k][k];
        #pragma capc profitability_region end

        #pragma capc profitability_region begin
        for (j = k + 1; j < N; j++)
        {
            R[k][j] = 0.0;

            for (i = 0; i < M; i++)
                R[k][j] += Q[i][k] * A[i][j];

            for (i = 0; i < M; i++)
                A[i][j] = A[i][j] - Q[i][k] * R[k][j];
        }
        #pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_gramschmidt();

    /* prevent complete dead-code elimination */
    printf("%f %f\n", R[0][0], Q[0][0]);

    return 0;
}