#include <stdio.h>

#define N 1000

double a[N];
double b[N];

int main()
{
    /* =========================================================
       REGION 1: Straight-line loop
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i] + 1.0;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 2: One if/else
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (i % 2 == 0)
            a[i] = b[i] + 1.0;
        else
            a[i] = b[i] - 1.0;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 3: Two independent if statements
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i];

        if (a[i] > 10.0)
            a[i] += 1.0;

        if (a[i] > 20.0)
            a[i] *= 2.0;
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 4: Nested if/else
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (i % 2 == 0)
        {
            if (b[i] > 50.0)
                a[i] = b[i] * 2.0;
            else
                a[i] = b[i] + 2.0;
        }
        else
        {
            a[i] = b[i] - 2.0;
        }
    }
    #pragma capc profitability_region end


    /* =========================================================
       REGION 5: Three-way if/else-if/else
       ========================================================= */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (b[i] < 10.0)
        {
            a[i] = b[i] + 1.0;
        }
        else if (b[i] < 100.0)
        {
            a[i] = b[i] * 2.0;
        }
        else
        {
            a[i] = b[i] - 1.0;
        }
    }
    #pragma capc profitability_region end


    printf("%lf\n", a[0]);

    return 0;
}