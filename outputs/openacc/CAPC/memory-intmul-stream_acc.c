#include <stdio.h>

#define N 10000000

int A[N];
int B[N];
int C[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma acc parallel loop copyout(A[0:N],B[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = i;
        B[i] = N - i;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Integer multiply + add
       ============================================================ */

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N],B[0:N]) copyout(C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] * 7 + B[i];
    }
#pragma capc profitability_region end

    printf("C[0] = %d\n", C[0]);
    printf("C[N-1] = %d\n", C[N - 1]);

    return 0;
}