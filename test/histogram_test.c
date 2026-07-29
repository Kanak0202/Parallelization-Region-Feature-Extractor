#include <stdio.h>
#define N 1000000
#define BINS 256

int main()
{
    int A[N], hist[BINS];
    int i;
    for (i = 0; i < N; i++) A[i] = i % BINS;
    for (i = 0; i < BINS; i++) hist[i] = 0;

    #pragma capc profitability_region begin
    for (i = 0; i < N; i++)
        hist[A[i]]++;
    #pragma capc profitability_region end

    printf("%d\n", hist[0]);
    return 0;
}