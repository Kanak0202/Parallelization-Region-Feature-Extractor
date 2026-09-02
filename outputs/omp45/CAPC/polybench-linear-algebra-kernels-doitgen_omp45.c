#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define NR 150
#define NQ 140
#define NP 160

double A[NR][NQ][NP];
double C4[NP][NP];
double sum[NP];

void init_array()
{
    for (int i = 0; i < NR; i++)
        for (int j = 0; j < NQ; j++)
            for (int k = 0; k < NP; k++)
                A[i][j][k] = (double)((i * j + k) % NP) / NP;

    for (int i = 0; i < NP; i++)
        for (int j = 0; j < NP; j++)
            C4[i][j] = (double)(i * j % NP) / NP;
}

void kernel_doitgen()
{
    int r, q, p, s;

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for collapse(2) private(sum) map(to:C4[0:NP][0:NP]) map(tofrom:A[0:NR][0:NQ][0:NP])
    for (r = 0; r < NR; r++)
    {
        for (q = 0; q < NQ; q++)
        {
            for (p = 0; p < NP; p++)
            {
                sum[p] = 0.0;

                for (s = 0; s < NP; s++)
                    sum[p] += A[r][q][s] * C4[s][p];
            }

            for (p = 0; p < NP; p++)
                A[r][q][p] = sum[p];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    init_array();
    kernel_doitgen();

    /* prevent complete dead-code elimination */
    printf("%f\n", A[0][0][0]);

    return 0;
}