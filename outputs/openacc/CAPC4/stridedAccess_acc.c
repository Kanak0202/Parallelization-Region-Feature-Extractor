#include <stdio.h>

#define N 10000000

double A[N];
double B[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
        A[i] = (double)(i + 1);

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N]) copyout(B[0:N-16])
    for (i = 0; i < N - 16; i++)
    {
        B[i] = A[i]
             + A[i + 2]
             + A[i + 4]
             + A[i + 8]
             + A[i + 16];
    }
#pragma capc profitability_region end

    printf("%f %f\n", B[0], B[N - 17]);

    return 0;
}