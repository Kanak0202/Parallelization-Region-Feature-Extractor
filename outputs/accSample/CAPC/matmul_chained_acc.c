#include <stdio.h>

#define N 2000

double a[N][N], b[N][N], c[N][N], d[N][N];

int main() {
    int i, j, k;

    #pragma acc enter data create(a[0:N][0:N], b[0:N][0:N], c[0:N][0:N], d[0:N][0:N])

    // Region 1: Initialization
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a, b, c, d)
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            a[i][j] = (double)(i + j) / N;
            b[i][j] = (double)(i - j) / N;
            c[i][j] = 0.0;
            d[i][j] = 0.0;
        }
    }
    #pragma capc profitability_region end
    #pragma acc wait

    // Region 2: Matrix Multiplication 1 (C = A * B)
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a, b, c)
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            double sum = 0.0;
            for (k = 0; k < N; k++) {
                sum += a[i][k] * b[k][j];
            }
            c[i][j] = sum;
        }
    }
    #pragma capc profitability_region end
    #pragma acc wait

    // Region 3: Matrix Multiplication 2 (D = C * A)
    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a, c, d)
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            double sum = 0.0;
            for (k = 0; k < N; k++) {
                sum += c[i][k] * a[k][j];
            }
            d[i][j] = sum;
        }
    }
    #pragma capc profitability_region end
    #pragma acc wait

    #pragma acc update self(d[0:N][0:N])
    printf("Check Result: d[0][0]=%f, d[%d][%d]=%f\n", d[0][0], N-1, N-1, d[N-1][N-1]);

    #pragma acc exit data delete(a[0:N][0:N], b[0:N][0:N], c[0:N][0:N], d[0:N][0:N])
    return 0;
}