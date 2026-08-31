#include <stdio.h>

#define N 10000000

int A[N];
int B[N];

int main()
{
    int i;
    long long sum = 0;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for map(from:A[0:N],B[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = i;
        B[i] = N - i;
    }
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Integer reduction
       ============================================================ */

#pragma capc profitability_region begin
#pragma omp target teams distribute parallel for reduction(+:sum) map(to:A[0:N],B[0:N]) map(tofrom:sum)
    for (i = 0; i < N; i++)
    {
        sum += A[i] + B[i];
    }
#pragma capc profitability_region end

    printf("sum = %lld\n", sum);

    return 0;
}