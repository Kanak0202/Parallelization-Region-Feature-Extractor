// test/indirect_test.c
#include <stdio.h>
#define SIZE 1000000

int main()
{
        double A[SIZE], B[SIZE], C[SIZE];
        int idx[SIZE];
        int i;

        for (i = 0; i < SIZE; ++i) {
                A[i] = (double)i;
                B[i] = (double)(i + 1);
                idx[i] = (SIZE - 1) - i;
        }

        #pragma capc profitability_region begin
        for (i = 0; i < SIZE; ++i)
        {
                C[i] = A[idx[i]] + B[i];
        }
        #pragma capc profitability_region end

        printf("C[0] = %f\n", C[0]);
        return 0;
}
