// test/reduction_test.c
#include <stdio.h>
#define SIZE 1000000

int main()
{
        double A[SIZE], B[SIZE];
        double sum = 0.0;
        int i;

        for (i = 0; i < SIZE; ++i) {
                A[i] = (double)i;
                B[i] = (double)(i + 1);
        }

        #pragma capc profitability_region begin
        for (i = 0; i < SIZE; ++i)
        {
                sum += A[i] * B[i];
        }
        #pragma capc profitability_region end

        printf("sum = %f\n", sum);
        return 0;
}
