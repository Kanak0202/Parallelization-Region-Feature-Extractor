#include <stdio.h>

#define N 128
#define SCALE 2.5

double A[N][N];
double B[N][N];
double C[N][N];

void initialize_matrices()
{
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            A[i][j] = i + j + 1.0;
            B[i][j] = i - j + 2.0;
            C[i][j] = 0.0;
        }
    }
}

/*----------------------------------------------------------*/
/* Matrix Addition */
/*----------------------------------------------------------*/
void matrix_add()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[i][j] = A[i][j] + B[i][j];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Subtraction */
/*----------------------------------------------------------*/
void matrix_subtract()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[i][j] = A[i][j] - B[i][j];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Scaling */
/*----------------------------------------------------------*/
void matrix_scale()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[i][j] = SCALE * A[i][j];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Copy */
/*----------------------------------------------------------*/
void matrix_copy()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[i][j] = A[i][j];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Transpose */
/*----------------------------------------------------------*/
void matrix_transpose()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[j][i] = A[i][j];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Multiplication (ijk) */
/*----------------------------------------------------------*/
void matrix_multiply()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            C[i][j] = 0.0;

            for (int k = 0; k < N; k++)
            {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Matrix Difference (tests branch counting) */
/*----------------------------------------------------------*/
void matrix_difference()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            if (A[i][j] > B[i][j])
                C[i][j] = A[i][j] - B[i][j];
            else
                C[i][j] = B[i][j] - A[i][j];
        }
    }
#pragma capc profitability_region end
}

int main()
{
    initialize_matrices();

    matrix_add();
    matrix_subtract();
    matrix_scale();
    matrix_copy();
    matrix_transpose();
    matrix_multiply();
    matrix_difference();

    printf("Checksum : %f\n", C[0][0]);

    return 0;
}