#include <stdio.h>
#include <stdlib.h>

#define N 2000

double A[N][N];
double u1[N];
double v1[N];
double u2[N];
double v2[N];
double w[N];
double x[N];
double y[N];
double z[N];

void init_array()
{
    double fn = (double)N;

    for (int i = 0; i < N; i++)
    {
        u1[i] = i;
        u2[i] = ((i + 1) / fn) / 2.0;
        v1[i] = ((i + 1) / fn) / 4.0;
        v2[i] = ((i + 1) / fn) / 6.0;
        y[i] = ((i + 1) / fn) / 8.0;
        z[i] = ((i + 1) / fn) / 9.0;

        x[i] = 0.0;
        w[i] = 0.0;

        for (int j = 0; j < N; j++)
            A[i][j] = (double)(i * j % N) / N;
    }
}

void kernel_gemver()
{
    int i, j;

    double alpha = 1.5;
    double beta = 1.2;

    /*
     * A = A + u1*v1^T + u2*v2^T
     *
     * Every A[i][j] is independent.
     */
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for collapse(2) map(to:u1[0:N],v1[0:N],u2[0:N],v2[0:N]) map(tofrom:A[0:N][0:N])
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            A[i][j] =
                A[i][j]
                + u1[i] * v1[j]
                + u2[i] * v2[j];
#pragma capc profitability_region end

    /*
     * x = x + beta * A^T * y
     *
     * Different i values update different x[i].
     * The j loop is a reduction into x[i].
     */
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:A[0:N][0:N],y[0:N]) map(tofrom:x[0:N])
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            x[i] += beta * A[j][i] * y[j];
#pragma capc profitability_region end

    /*
     * x = x + z
     *
     * Each x[i] is independent.
     */
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:z[0:N]) map(tofrom:x[0:N])
    for (i = 0; i < N; i++)
        x[i] += z[i];
#pragma capc profitability_region end

    /*
     * w = w + alpha * A * x
     *
     * Different i values update different w[i].
     * The j loop is a reduction into w[i].
     */
#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(to:A[0:N][0:N],x[0:N]) map(tofrom:w[0:N])
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            w[i] += alpha * A[i][j] * x[j];
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_gemver();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", w[0]);

    return 0;
}