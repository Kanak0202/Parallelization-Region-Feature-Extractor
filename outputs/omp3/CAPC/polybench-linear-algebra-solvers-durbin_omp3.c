#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 2000

double r[N];
double y[N];
double z[N];

void init_array()
{
    for (int i = 0; i < N; i++)
        r[i] = (double)(N + 1 - i);
}

void kernel_durbin()
{
    int i, k;
    double alpha;
    double beta;
    double sum;


    y[0] = -r[0];
    beta = 1.0;
    alpha = -r[0];

    for (k = 1; k < N; k++)
    {
        beta = (1.0 - alpha * alpha) * beta;

        sum = 0.0;
        #pragma capc profitability_region begin
        #pragma omp parallel for private(i) reduction(+:sum)
        for (i = 0; i < k; i++)
            sum += r[k - i - 1] * y[i];
        #pragma capc profitability_region end

        alpha = -(r[k] + sum) / beta;

        #pragma capc profitability_region begin
        #pragma omp parallel for private(i)
        for (i = 0; i < k; i++)
            z[i] = y[i] + alpha * y[k - i - 1];
        #pragma capc profitability_region end

        #pragma capc profitability_region begin
        #pragma omp parallel for private(i)
        for (i = 0; i < k; i++)
            y[i] = z[i];
        #pragma capc profitability_region end

        y[k] = alpha;
    }
}

int main()
{
    init_array();
    kernel_durbin();

    /* prevent complete dead-code elimination */
    printf("%f\n", y[N - 1]);

    return 0;
}