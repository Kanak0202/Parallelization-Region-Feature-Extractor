#include <stdio.h>

#define N 512
#define M 512

double A[N][M];
double B[N][M];

double X[N];
double Y[N];

void initialize()
{
    for (int i = 0; i < N; i++)
    {
        X[i] = (double)(i + 1);
        Y[i] = 0.0;

        for (int j = 0; j < M; j++)
        {
            A[i][j] = (double)(i + j);
            B[i][j] = 0.0;
        }
    }
}

/*----------------------------------------------------------*/
/* 1D Three Point Stencil */
/*----------------------------------------------------------*/
void stencil_1d_three_point()
{
#pragma capc profitability_region begin
    for (int i = 1; i < N - 1; i++)
    {
        Y[i] = X[i - 1] + X[i] + X[i + 1];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* 1D Five Point Stencil */
/*----------------------------------------------------------*/
void stencil_1d_five_point()
{
#pragma capc profitability_region begin
    for (int i = 2; i < N - 2; i++)
    {
        Y[i] =
            X[i - 2] +
            X[i - 1] +
            X[i] +
            X[i + 1] +
            X[i + 2];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* 2D Five Point Stencil */
/*----------------------------------------------------------*/
void stencil_2d_five_point()
{
#pragma capc profitability_region begin
    for (int i = 1; i < N - 1; i++)
    {
        for (int j = 1; j < M - 1; j++)
        {
            B[i][j] =
                A[i][j] +
                A[i - 1][j] +
                A[i + 1][j] +
                A[i][j - 1] +
                A[i][j + 1];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* 2D Nine Point Stencil */
/*----------------------------------------------------------*/
void stencil_2d_nine_point()
{
#pragma capc profitability_region begin
    for (int i = 1; i < N - 1; i++)
    {
        for (int j = 1; j < M - 1; j++)
        {
            B[i][j] =
                A[i][j] +
                A[i - 1][j] +
                A[i + 1][j] +
                A[i][j - 1] +
                A[i][j + 1] +
                A[i - 1][j - 1] +
                A[i - 1][j + 1] +
                A[i + 1][j - 1] +
                A[i + 1][j + 1];
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Jacobi Iteration */
/*----------------------------------------------------------*/
void jacobi_iteration()
{
#pragma capc profitability_region begin
    for (int i = 1; i < N - 1; i++)
    {
        for (int j = 1; j < M - 1; j++)
        {
            B[i][j] =
                0.2 * (
                    A[i][j] +
                    A[i - 1][j] +
                    A[i + 1][j] +
                    A[i][j - 1] +
                    A[i][j + 1]);
        }
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Heat Diffusion */
/*----------------------------------------------------------*/
void heat_diffusion()
{
#pragma capc profitability_region begin
    for (int i = 1; i < N - 1; i++)
    {
        for (int j = 1; j < M - 1; j++)
        {
            B[i][j] =
                A[i][j] +
                0.25 * (
                    A[i - 1][j] +
                    A[i + 1][j] +
                    A[i][j - 1] +
                    A[i][j + 1] -
                    4.0 * A[i][j]);
        }
    }
#pragma capc profitability_region end
}

int main()
{
    initialize();

    stencil_1d_three_point();
    stencil_1d_five_point();
    stencil_2d_five_point();
    stencil_2d_nine_point();
    jacobi_iteration();
    heat_diffusion();

    printf("Checksum : %f\n", B[1][1] + Y[1]);

    return 0;
}