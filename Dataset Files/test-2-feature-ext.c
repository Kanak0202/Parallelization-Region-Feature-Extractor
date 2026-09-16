#include <stdio.h>

#define N 1000

double a[N];
double b[N];

int main()
{
    /*
     * REGION 1
     *
     * Straight-line loop
     */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i] + 1.0;
    }
    #pragma capc profitability_region end


    /*
     * REGION 2
     *
     * Single if
     */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        a[i] = b[i];

        if (a[i] > 10.0)
        {
            a[i] = a[i] + 1.0;
        }
    }
    #pragma capc profitability_region end


    /*
     * REGION 3
     *
     * if / else
     */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (b[i] > 10.0)
        {
            a[i] = b[i] + 1.0;
        }
        else
        {
            a[i] = b[i] - 1.0;
        }
    }
    #pragma capc profitability_region end


    /*
     * REGION 4
     *
     * if / else-if / else
     */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (b[i] < 10.0)
        {
            a[i] = 1.0;
        }
        else if (b[i] < 20.0)
        {
            a[i] = 2.0;
        }
        else
        {
            a[i] = 3.0;
        }
    }
    #pragma capc profitability_region end


    /*
     * REGION 5
     *
     * Nested conditionals
     */
    #pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (b[i] > 10.0)
        {
            if (b[i] > 20.0)
            {
                a[i] = 3.0;
            }
            else
            {
                a[i] = 2.0;
            }
        }
        else
        {
            a[i] = 1.0;
        }
    }
    #pragma capc profitability_region end

    printf("%lf\n", a[0]);

    return 0;
}