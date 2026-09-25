#include <stdio.h>

#define N 10000000

double A[N];
double B[N];
double C[N];

static inline double transform(double x, double y)
{
    double t;

    t = x * x;
    t = t + y * 2.0;
    t = t - x * 0.5;

    return t;
}

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)(i % 100) + 1.0;
        B[i] = (double)(i % 50) + 2.0;
    }

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        C[i] = transform(A[i], B[i]);
    }
#pragma capc profitability_region end

    printf("%f %f\n", C[0], C[N - 1]);

    return 0;
}