#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NI 800
#define NJ 900
#define NK 1000
#define NL 1100
#define NM 1200

double A[NI][NK];
double B[NK][NJ];
double C[NJ][NM];
double D[NM][NL];

double E[NI][NJ];
double F[NJ][NL];
double G[NI][NL];

void init_array()
{
    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NK; j++)
            A[i][j] = (double)((i * j + 1) % NI) / (5 * NI);

    for (int i = 0; i < NK; i++)
        for (int j = 0; j < NJ; j++)
            B[i][j] = (double)((i * (j + 1) + 2) % NJ) / (5 * NJ);

    for (int i = 0; i < NJ; i++)
        for (int j = 0; j < NM; j++)
            C[i][j] = (double)(i * (j + 3) % NL) / (5 * NL);

    for (int i = 0; i < NM; i++)
        for (int j = 0; j < NL; j++)
            D[i][j] = (double)((i * (j + 2) + 2) % NK) / (5 * NK);
}

void kernel_3mm()
{
    int i, j, k;

#pragma capc profitability_region begin
    for (i = 0; i < NI; i++)
    {
        for (j = 0; j < NJ; j++)
        {
            E[i][j] = 0.0;

            for (k = 0; k < NK; k++)
                E[i][j] += A[i][k] * B[k][j];
        }
    }
#pragma capc profitability_region end

#pragma capc profitability_region begin
    for (i = 0; i < NJ; i++)
    {
        for (j = 0; j < NL; j++)
        {
            F[i][j] = 0.0;

            for (k = 0; k < NM; k++)
                F[i][j] += C[i][k] * D[k][j];
        }
    }
#pragma capc profitability_region end

#pragma capc profitability_region begin
    for (i = 0; i < NI; i++)
    {
        for (j = 0; j < NL; j++)
        {
            G[i][j] = 0.0;

            for (k = 0; k < NJ; k++)
                G[i][j] += E[i][k] * F[k][j];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_3mm();

    /* prevent complete dead-code elimination */
    printf("%f\n", G[0][0]);

    return 0;
}