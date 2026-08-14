#include <stdio.h>
#include <stdlib.h>

#define N 67108864 // ~536 MB per double array (4 arrays = ~2.15 GB total)

static double a[N], b[N], c[N], d[N];

int main() {
    int i;
    double scalar = 3.0;

    #pragma acc enter data create(a[0:N], b[0:N], c[0:N], d[0:N])

    // Region 1: Parallel Initialization
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(a[0:N], b[0:N], c[0:N], d[0:N])
    for (i = 0; i < N; i++) {
        a[i] = 1.0;
        b[i] = 2.0;
        c[i] = 0.5;
        d[i] = 0.0;
    }
    #pragma capc profitability_region end
    #pragma acc wait

    // Region 2: STREAM Triad Operation
    #pragma capc profitability_region begin
    #pragma acc parallel loop present(a[0:N], b[0:N], c[0:N], d[0:N])
    for (i = 0; i < N; i++) {
        d[i] = a[i] + scalar * b[i] + c[i];
    }
    #pragma capc profitability_region end
    #pragma acc wait

    return 0;
}