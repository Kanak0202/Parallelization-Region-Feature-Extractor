#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(2 * i);
        C[i] = 0.0;
    }

    #pragma acc enter data copyin(A[0:N], B[0:N]) create(C[0:N])

    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }
    #pragma capc profitability_region end

    #pragma acc update self(C[0:N])

    printf("C[0] = %f\n", C[0]);
    printf("C[N-1] = %f\n", C[N-1]);

    #pragma acc exit data delete(A[0:N], B[0:N], C[0:N])

    return 0;
}