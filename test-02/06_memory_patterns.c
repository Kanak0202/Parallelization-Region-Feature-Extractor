#include <stdio.h>

#define N 1024

int A[N];
int B[N];
int C[N];
int IDX[N];

void initialize()
{
    for (int i = 0; i < N; i++)
    {
        A[i] = i;
        B[i] = 2 * i;
        C[i] = 0;
        IDX[i] = (i * 7) % N;
    }
}

/*----------------------------------------------------------*/
/* Sequential Copy */
/*----------------------------------------------------------*/
void sequential_copy()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        C[i] = A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Sequential Add */
/*----------------------------------------------------------*/
void sequential_add()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Stride-2 Access */
/*----------------------------------------------------------*/
void stride_two()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N / 2; i++)
    {
        C[i] = A[2 * i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Stride-4 Access */
/*----------------------------------------------------------*/
void stride_four()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N / 4; i++)
    {
        C[i] = A[4 * i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Gather Pattern (Indirect Access) */
/*----------------------------------------------------------*/
void gather_access()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        C[i] = A[IDX[i]];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Scatter Pattern (Indirect Store) */
/*----------------------------------------------------------*/
void scatter_access()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        C[IDX[i]] = A[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Multiple Array Access */
/*----------------------------------------------------------*/
void multiple_arrays()
{
#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        C[i] = A[i] + B[i] + C[i];
    }
#pragma capc profitability_region end
}

/*----------------------------------------------------------*/
/* Read-Only Kernel */
/*----------------------------------------------------------*/
void read_only()
{
    volatile int sink = 0;

#pragma capc profitability_region begin
    for (int i = 0; i < N; i++)
    {
        sink += A[i];
    }
#pragma capc profitability_region end
}

int main()
{
    initialize();

    sequential_copy();
    sequential_add();
    stride_two();
    stride_four();
    gather_access();
    scatter_access();
    multiple_arrays();
    read_only();

    printf("%d\n", C[0]);

    return 0;
}