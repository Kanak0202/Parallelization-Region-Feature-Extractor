#include <stdio.h>

#define N 400000

double A[N];
double B[N];
double C[N];

int index_array[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = (double)i;
        B[i] = (double)(N - i);
        C[i] = 0.0;

        index_array[i] = (i * 17) % N;
    }

    #pragma acc enter data \
        copyin(A[0:N], B[0:N], index_array[0:N]) \
        create(C[0:N])

    /* Region 1: indirect gather */
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        present(A[0:N], C[0:N], index_array[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = A[index_array[i]];
    }
    #pragma capc profitability_region end

    /* Region 2: another indirect read */
    #pragma capc profitability_region begin
    #pragma acc parallel loop \
        present(B[0:N], C[0:N], index_array[0:N])
    for (i = 0; i < N; i++)
    {
        C[i] = C[i] + B[index_array[i]];
    }
    #pragma capc profitability_region end

    /* Region 3: sequential write, multiple reads */
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(A[0:N], B[0:N], C[0:N])
    for (i = 0; i < N; i++)
    {
        A[i] = B[i] + C[i];
    }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:N])

    printf("A[N-1] = %f\n", A[N-1]);

    #pragma acc exit data delete(A[0:N],B[0:N],C[0:N],index_array[0:N])

    return 0;
}