#include <stdio.h>

#define N 300000

double A[N];
double B[N];
double C[N];
double D[N];
double E[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = i * 0.1;
        B[i] = i * 0.2;
        C[i] = 0.0;
        D[i] = 0.0;
        E[i] = 0.0;
    }

    #pragma acc enter data copyin(A[0:N], B[0:N]) \
        create(C[0:N], D[0:N], E[0:N])

    /* Region 1 */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }
    #pragma capc profitability_region end

    /* Region 2 */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], C[0:N], D[0:N])
    for (i = 0; i < N; i++)
    {
        D[i] = C[i] - A[i];
    }
    #pragma capc profitability_region end

    /* Region 3 */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(B[0:N], D[0:N], E[0:N])
    for (i = 0; i < N; i++)
    {
        E[i] = D[i] + B[i];
    }
    #pragma capc profitability_region end

    /* Region 4 */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], C[0:N], E[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = C[i] + E[i];
    }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:N])

    printf("A[N-1] = %f\n", A[N-1]);

    #pragma acc exit data delete(A[0:N], B[0:N], C[0:N], D[0:N], E[0:N])

    return 0;
}