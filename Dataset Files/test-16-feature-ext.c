#include <stdio.h>
#include <math.h>
#define N 1000

int main() {
    double sum = 0.0;

    // CAPC_BEGIN
    #pragma capc profitability_region begin

    for (int i = 0; i < N; i++) {
        sum += fmod((double)i, 7.5);
    }

    #pragma capc profitability_region end
    // CAPC_END

    printf("%f\n", sum);

    return 0;
}