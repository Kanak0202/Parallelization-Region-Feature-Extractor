#include <stdio.h>

#define N 67108864 // ~536 MB per double array (3 arrays = ~1.6 GB total)

static double a[N];
static double b[N];
static double c[N];

int main() {
    int i;

    // Region 1: Initialization with Device -> Host copyout
    #pragma capc profitability_region begin
    #pragma acc parallel loop copyout(a[0:N], b[0:N])
    for (i = 0; i < N; i++) {
        a[i] = 1.5;
        b[i] = 2.5;
    }
    #pragma capc profitability_region end

    // Region 2: Vector Add with explicit Host -> Device copyin and Device -> Host copyout
    #pragma capc profitability_region begin
    #pragma acc parallel loop copyin(a[0:N], b[0:N]) copyout(c[0:N])
    for (i = 0; i < N; i++) {
        c[i] = a[i] + b[i];
    }
    #pragma capc profitability_region end

    return 0;
}