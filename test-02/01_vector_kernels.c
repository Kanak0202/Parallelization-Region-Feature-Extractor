#include <stdio.h>

#define N 1024
#define A_VAL 2.0
#define B_VAL 3.0
#define SCALAR 2.5

double A[N];
double B[N];
double C[N];
double D[N];

double dot_result = 0.0;
double sum_result = 0.0;

void initialize_arrays()
{
    for (int i = 0; i < N; i++)
    {
        A[i] = A_VAL + i * 0.01;
        B[i] = B_VAL + i * 0.02;
        C[i] = 0.0;
        D[i] = 1.0;
    }
}

/*----------------------------------------------------------*/
/* Vector Addition */
/*----------------------------------------------------------*/
void vector_add()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = A[i] + B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Vector Subtraction */
/*----------------------------------------------------------*/
void vector_subtract()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = A[i] - B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Element-wise Multiplication */
/*----------------------------------------------------------*/
void vector_multiply()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = A[i] * B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* SAXPY */
/* C = a*A + B */
/*----------------------------------------------------------*/
void saxpy()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = SCALAR * A[i] + B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Vector Scaling */
/*----------------------------------------------------------*/
void vector_scale()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = SCALAR * A[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Vector Copy */
/*----------------------------------------------------------*/
void vector_copy()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = A[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Vector Triad */
/* A = B + scalar*C */
/*----------------------------------------------------------*/
void vector_triad()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        A[i] = B[i] + SCALAR * C[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Dot Product */
/*----------------------------------------------------------*/
void dot_product()
{
    dot_result = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        dot_result += A[i] * B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Sum Reduction */
/*----------------------------------------------------------*/
void sum_reduction()
{
    sum_result = 0.0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum_result += A[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Element-wise Square */
/*----------------------------------------------------------*/
void vector_square()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = A[i] * A[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Weighted Vector Addition */
/* C = 0.25*A + 0.75*B */
/*----------------------------------------------------------*/
void weighted_add()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        C[i] = 0.25 * A[i] + 0.75 * B[i];
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Fused Arithmetic */
/* D = A*B + C */
/*----------------------------------------------------------*/
void fused_arithmetic()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        D[i] = A[i] * B[i] + C[i];
#pragma capc profitability_region end
}

int main()
{
    initialize_arrays();

    vector_add();
    vector_subtract();
    vector_multiply();
    saxpy();
    vector_scale();
    vector_copy();
    vector_triad();
    dot_product();
    sum_reduction();
    vector_square();
    weighted_add();
    fused_arithmetic();

    printf("Dot Product : %f\n", dot_result);
    printf("Sum         : %f\n", sum_result);
    printf("Checksum    : %f\n", D[0]);

    return 0;
}