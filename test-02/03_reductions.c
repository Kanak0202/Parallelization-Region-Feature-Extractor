#include <stdio.h>

#define N 1024

double A[N];
double B[N];
double C[N];

double sum;
double product;
double maximum;
double minimum;
double average;
double l2norm;

void initialize_arrays()
{
    for (int i = 0; i < N; i++)
    {
        A[i] = (double)(i + 1);
        B[i] = (double)(i % 17 + 1);
        C[i] = 0.0;
    }
}

/*----------------------------------------------------------*/
/* Sum Reduction */
/*----------------------------------------------------------*/
void sum_reduction()
{
    sum = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        sum += A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Product Reduction */
/*----------------------------------------------------------*/
void product_reduction()
{
    product = 1.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        product *= 1.000001;
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Maximum Reduction */
/*----------------------------------------------------------*/
void max_reduction()
{
    maximum = A[0];

#pragma capc profitability_region begin
    for (int i = 1; i < N; i++)
    {
        if (A[i] > maximum)
            maximum = A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Minimum Reduction */
/*----------------------------------------------------------*/
void min_reduction()
{
    minimum = A[0];

#pragma capc profitability_region begin
    for (int i = 1; i < N; i++)
    {
        if (A[i] < minimum)
            minimum = A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Average */
/*----------------------------------------------------------*/
void average_reduction()
{
    average = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        average += A[i];
    }
#pragma capc profitability_region end

    average /= N;
}

/*----------------------------------------------------------*/
/* L2 Norm */
/*----------------------------------------------------------*/
void l2_norm()
{
    l2norm = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        l2norm += A[i] * A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Sum of Absolute Differences */
/*----------------------------------------------------------*/
void sad_reduction()
{
    sum = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        if (A[i] > B[i])
            sum += A[i] - B[i];
        else
            sum += B[i] - A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Weighted Sum */
/*----------------------------------------------------------*/
void weighted_sum()
{
    sum = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        sum += 0.25 * A[i] + 0.75 * B[i];
    }
#pragma capc profitability_region end
}

int main()
{
    initialize_arrays();

    sum_reduction();
    product_reduction();
    max_reduction();
    min_reduction();
    average_reduction();
    l2_norm();
    sad_reduction();
    weighted_sum();

    printf("Sum      : %f\n", sum);
    printf("Product  : %f\n", product);
    printf("Maximum  : %f\n", maximum);
    printf("Minimum  : %f\n", minimum);
    printf("Average  : %f\n", average);
    printf("L2 Norm  : %f\n", l2norm);

    return 0;
}