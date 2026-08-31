#include <stdio.h>

#define N 10000000

int A[N];
int B[N];
int index_array[N];

int main()
{
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma acc parallel loop copyout(A[0:N],B[0:N],index_array[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = i + 100;
        B[i] = N - i + 100;
        index_array[i] = (i * 17) % N;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Indirect read + integer division
       ============================================================ */

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(B[0:N],index_array[0:N]) copy(A[0:N])
    for (i = 0; i < N; i++)
    {
        int value = B[index_array[i]];

        if (value > 100)
        {
            A[i] = value / 3 + A[i];
        }
        else
        {
            A[i] = value + A[i];
        }
    }
#pragma capc profitability_region end

    printf("A[0] = %d\n", A[0]);
    printf("A[N-1] = %d\n", A[N - 1]);

    return 0;
}