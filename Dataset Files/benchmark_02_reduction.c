#include <stdio.h>
#include <stdlib.h>

#define N 1000000

int main()
{
    float *A = malloc(N*sizeof(float));
#pragma capc profitability_region begin
    for(int i=0;i<N;i++)
        A[i] = 1.0f;
#pragma capc profitability_region end

    float sum = 0.0f;
#pragma capc profitability_region begin

    for(int i=0;i<N;i++)
        sum += A[i];
#pragma capc profitability_region end

    printf("%f\n", sum);

    free(A);

    return 0;
}
