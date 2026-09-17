#include <stdio.h>

#define N 1000

double a[N];
double b[N];

int main()
{
    /* =========================================================
       REGION 1: Integer arithmetic using loop index
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        int x = i + 2;
        int y = x * 3;
        a[i] = b[i] + y;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 2: Multiple integer arithmetic operations
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        int x = i * 2;
        int y = x + 10;
        int z = y - 5;
        a[i] = b[i] + z;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 3: Floating-point arithmetic
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        double x = b[i] + 2.5;
        double y = x * 3.0;
        double z = y - 1.5;
        a[i] = z;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 4: Mixed integer and floating-point arithmetic
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        int x = i + 1;
        int y = x * 2;
        double z = b[i] + (double)y;
        z = z * 1.5;
        a[i] = z - 2.0;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 5: Arithmetic-heavy loop body
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        double x = b[i];

        x = x + 2.0;
        x = x * 3.0;
        x = x - 4.0;
        x = x * 1.5;
        x = x + 10.0;
        x = x / 2.0;

        a[i] = x;
    }
    #pragma capc profitability_region end


    printf("%lf\n", a[0]);

    return 0;
}