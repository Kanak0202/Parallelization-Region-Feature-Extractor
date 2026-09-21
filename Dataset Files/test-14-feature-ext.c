#include <stdio.h>
#define N 1000

int main() {

    int sum = 0;

    // CAPC_BEGIN
    #pragma capc profitability_region begin

    for (int i = 0; i < N; i++) {
        sum += i % 7;
    }

    #pragma capc profitability_region end
    // CAPC_END

    printf("%d\n", sum);

    return 0;
}