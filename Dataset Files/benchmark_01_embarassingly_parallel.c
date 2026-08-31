#include <stdio.h>
#include <stdlib.h>

#define N 1000000

int main()
{
    float *A = malloc(N*sizeof(float));
    float *B = malloc(N*sizeof(float));
    float *C = malloc(N*sizeof(float));
    #pragma capc profitability_region begin
    for(int i=0;i<N;i++)
    {
        B[i] = i;
        C[i] = 2*i;
    }
    #pragma capc profitability_region end
        #pragma capc profitability_region begin

    for(int i=0;i<N;i++)
        A[i] = B[i] + C[i];
    #pragma capc profitability_region end

    printf("%f\n", A[N-1]);

    free(A);
    free(B);
    free(C);

    return 0;
}
