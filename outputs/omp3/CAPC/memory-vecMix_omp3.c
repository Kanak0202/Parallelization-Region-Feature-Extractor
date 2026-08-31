#include <stdio.h>

#define N 10000000

int index_array[N];

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
    {
        index_array[i] = (i * 13 + 7) % N;

        A[i] = (double)i;
        B[i] = (double)(N - i);
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Mixed computation
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
    {
        int idx = index_array[i];
        int offset = (idx * 3) + 1;

        if (offset > N / 2)
        {
            C[i] = A[i] + 2.5 * B[idx];
        }
        else
        {
            C[i] = A[i] - 1.5 * B[idx];
        }
    }
#pragma capc profitability_region end

    printf("C[0] = %f\n", C[0]);
    printf("C[N-1] = %f\n", C[N - 1]);

    return 0;
}