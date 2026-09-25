#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];
double D[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = 1.001 + (double)(i % 100);
        B[i] = 0.991 + (double)(i % 50);
        C[i] = 0.5 + (double)(i % 25);
        D[i] = 1.0;
    }

#pragma capc profitability_region begin
#pragma acc parallel loop copyin(A[0:N], B[0:N], C[0:N]) copyout(D[0:N])
    for (i = 0; i < N; i++)
    {
        double x = A[i];
        double y = B[i];
        double z = C[i];

        x = x * y + z;
        x = x * 1.0001 + y;
        x = x * 0.9999 + z;
        x = x * y + 2.0;
        x = x * z + 3.0;

        D[i] = x;
    }
#pragma capc profitability_region end

    printf("%f %f\n", D[0], D[N - 1]);

    return 0;
}