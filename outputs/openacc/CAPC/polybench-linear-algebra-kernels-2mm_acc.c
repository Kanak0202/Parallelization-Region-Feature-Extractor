#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NI 800
#define NJ 900
#define NK 1100
#define NL 1200

double A[NI][NK];
double B[NK][NJ];
double C[NJ][NL];
double D[NI][NL];
double tmp[NI][NJ];

void init_array()
{
    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NK; j++)
            A[i][j] = (double)((i * j + 1) % NI) / NI;

    for (int i = 0; i < NK; i++)
        for (int j = 0; j < NJ; j++)
            B[i][j] = (double)(i * (j + 1) % NJ) / NJ;

    for (int i = 0; i < NJ; i++)
        for (int j = 0; j < NL; j++)
            C[i][j] = (double)((i * (j + 3) + 1) % NL) / NL;

    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NL; j++)
            D[i][j] = (double)(i * (j + 2) % NK) / NK;
}

void kernel_2mm()
{
    int i, j, k;

    double alpha = 1.5;
    double beta = 1.2;

#pragma capc profitability_region begin
#pragma acc parallel loop collapse(2) copyin(A[0:NI][0:NK],B[0:NK][0:NJ]) copyout(tmp[0:NI][0:NJ])
    for (i = 0; i < NI; i++)
    {
        for (j = 0; j < NJ; j++)
        {
            tmp[i][j] = 0.0;

            for (k = 0; k < NK; k++)
                tmp[i][j] += alpha * A[i][k] * B[k][j];
        }
    }
#pragma capc profitability_region end

#pragma capc profitability_region begin
#pragma acc parallel loop collapse(2) copyin(tmp[0:NI][0:NJ],C[0:NJ][0:NL]) copy(D[0:NI][0:NL])
    for (i = 0; i < NI; i++)
    {
        for (j = 0; j < NL; j++)
        {
            D[i][j] *= beta;

            for (k = 0; k < NJ; k++)
                D[i][j] += tmp[i][k] * C[k][j];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_2mm();

    /* prevent complete dead-code elimination */
    printf("%f\n", D[0][0]);

    return 0;
}