#include <stdio.h>

#define N 500

double A[N][N];
double B[N][N];
double C[N][N];
double D[N][N];
double R[N][N];

int main()
{
    int i, j, k;

    #pragma acc enter data create( \
        A[0:N][0:N], \
        B[0:N][0:N], \
        C[0:N][0:N], \
        D[0:N][0:N], \
        R[0:N][0:N])

    /*
     * Region 1
     * Pure producer / initialization
     */
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) \
        present(A[0:N][0:N], B[0:N][0:N], \
                C[0:N][0:N], D[0:N][0:N], \
                R[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = 0.1 * i + j;
            B[i][j] = 0.2 * j + i;
            C[i][j] = 0.3 * i + j;
            D[i][j] = 0.4 * j + i;
            R[i][j] = 0.0;
        }
    }
    #pragma capc profitability_region end


    /*
     * Region 2
     * Expensive compute
     */
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) \
        present(A[0:N][0:N], B[0:N][0:N], R[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                R[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    #pragma capc profitability_region end


    /*
     * Region 3
     * Depends on R but predecessor Region 2 must NOT
     * be serially replayed in standalone generation.
     */
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) \
        present(C[0:N][0:N], D[0:N][0:N], R[0:N][0:N])
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                R[i][j] += C[i][k] * D[k][j];
            }
        }
    }
    #pragma capc profitability_region end

    #pragma acc update self(R[0:N][0:N])

    printf("R[0][0] = %f\n", R[0][0]);

    #pragma acc exit data delete( \
        A[0:N][0:N], \
        B[0:N][0:N], \
        C[0:N][0:N], \
        D[0:N][0:N], \
        R[0:N][0:N])

    return 0;
}