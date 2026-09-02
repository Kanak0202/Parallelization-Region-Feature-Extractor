#include <stdio.h>
#include <stdlib.h>

#define NI 1000
#define NJ 1100
#define NK 1200

double C[NI][NJ];
double A[NI][NK];
double B[NK][NJ];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NJ; j++)
            C[i][j] = (double)((i * j + 1) % NI) / NI;

    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NK; j++)
            A[i][j] = (double)(i * (j + 1) % NK) / NK;

    for (int i = 0; i < NK; i++)
        for (int j = 0; j < NJ; j++)
            B[i][j] = (double)(i * (j + 2) % NJ) / NJ;
}

void kernel_gemm()
{
    int i, j, k;

    double alpha = 1.5;
    double beta = 1.2;

    /*
     * C = alpha * A * B + beta * C
     *
     * Each C[i][j] is independent of every other
     * C[i][j]. The k loop is a reduction into C[i][j].
     */
#pragma capc profitability_region begin
#pragma omp parallel for private(i,j,k)
    for (i = 0; i < NI; i++)
    {
#pragma omp parallel for private(j)
        for (j = 0; j < NJ; j++)
            C[i][j] *= beta;

        for (k = 0; k < NK; k++)
        {
#pragma omp parallel for private(j)
            for (j = 0; j < NJ; j++)
                C[i][j] += alpha * A[i][k] * B[k][j];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_gemm();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}