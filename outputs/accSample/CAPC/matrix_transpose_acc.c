#include <stdio.h>

#define N 10000 // 10000 x 10000 doubles = 800 MB per matrix (2 matrices = 1.6 GB total)

static double A[N][N];
static double B[N][N];

int main() {
    int i, j;

    #pragma acc enter data create(A[0:N][0:N], B[0:N][0:N])

    // Region 1: Matrix Initialization
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(A[0:N][0:N], B[0:N][0:N])
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            A[i][j] = (double)(i * N + j);
            B[i][j] = 0.0;
        }
    }
    #pragma capc profitability_region end
    #pragma acc wait

    // Region 2: Matrix Transpose (Strided Memory Access)
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(A[0:N][0:N], B[0:N][0:N])
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            B[j][i] = A[i][j];
        }
    }
    #pragma capc profitability_region end
    #pragma acc wait

    return 0;
}