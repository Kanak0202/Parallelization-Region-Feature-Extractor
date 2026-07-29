//==============================================================
// 04_fma_validation.c
//
// Validation suite for FMA detection
//
// Expected:
// Region 0 : FMA = 1
// Region 1 : FMA = 1
// Region 2 : FMA = 0
// Region 3 : FMA = 0
// Region 4 : FMA = 0
// Region 5 : FMA = 0
// Region 6 : FMA = 1
//==============================================================

#include <math.h>

#define N 1024

double A[N], B[N], C[N], D[N];
double sum;

//--------------------------------------------------------------
// Region 0
// Simple multiply-add
// Should detect one FMA
//--------------------------------------------------------------
void simple_fma()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum += A[i] * B[i];
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 1
// Self multiply
// Should detect one FMA
//--------------------------------------------------------------
void square_accumulate()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum += A[i] * A[i];
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 2
// Two multiplies
// Should NOT detect FMA
//--------------------------------------------------------------
void weighted_sum()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum += 0.25 * A[i] + 0.75 * B[i];
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 3
// Two independent multiplies
// Should NOT detect FMA
//--------------------------------------------------------------
void double_product()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum += A[i] * B[i] + C[i] * D[i];
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 4
// No multiplication
// Should NOT detect FMA
//--------------------------------------------------------------
void absolute_difference()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum += fabs(A[i] - B[i]);
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 5
// Pure multiply
// Should NOT detect FMA
//--------------------------------------------------------------
void multiply_only()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum = A[i] * B[i];
#pragma capc profitability_region end
}

//--------------------------------------------------------------
// Region 6
// FMA inside reduction
// Should detect one FMA
//--------------------------------------------------------------
void reduction_fma()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
        sum = sum + A[i] * B[i];
#pragma capc profitability_region end
}

int main()
{
    for (int i = 0; i < N; i++)
    {
        A[i] = i * 1.1;
        B[i] = i * 2.2;
        C[i] = i * 3.3;
        D[i] = i * 4.4;
    }

    sum = 0.0;

    simple_fma();
    square_accumulate();
    weighted_sum();
    double_product();
    absolute_difference();
    multiply_only();
    reduction_fma();

    return 0;
}