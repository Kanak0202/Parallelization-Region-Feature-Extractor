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
        A[i] = (double)(i + 1);
        B[i] = (double)((i % 101) + 1);
    }

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        double a = A[i];
        double b = B[i];

        double x = (a > b) ? (a / b) : (b / a);

        if (x > 10.0)
            C[i] = x * 0.5;
        else
            C[i] = x + 3.0;
    }
#pragma capc profitability_region end

    printf("%f %f\n", C[0], C[N - 1]);

    return 0;
}