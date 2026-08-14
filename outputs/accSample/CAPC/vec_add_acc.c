#include <stdio.h>
#include <stdlib.h>

#define N 10000000

double a[N], b[N], c[N];

int main() {
    int i;

    #pragma acc enter data create(a[0:N], b[0:N], c[0:N])

    // Region 1: Array Initialization
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(a, b, c)
    for (i = 0; i < N; i++) {
        a[i] = 1.0 * i;
        b[i] = 2.0 * i;
        c[i] = 0.0;
    }
    #pragma capc profitability_region end
    #pragma acc wait

    // Region 2: Vector Addition Computation
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(a, b, c)
    for (i = 0; i < N; i++) {
        c[i] = a[i] + b[i];
    }
    #pragma capc profitability_region end
    #pragma acc wait

    #pragma acc update self(c[0:N])

    printf("Check c[0]=%f, c[%d]=%f\n", c[0], N - 1, c[N - 1]);

    #pragma acc exit data delete(a[0:N], b[0:N], c[0:N])
    return 0;
}