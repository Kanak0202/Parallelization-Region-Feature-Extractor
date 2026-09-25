#include <stdio.h>

#define N 10000000

int A[N];
int B[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
        A[i] = i * 17 + 123;

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        int x = A[i];

        int r1 = x & 255;
        int r2 = (x >> 4) & 127;
        int r3 = x ^ (x << 3);
        int r4 = x | 1024;

        int m = x % 97;
        int d = x / 13;

        int value = (m > 40) ? (d + r1) : (d - r2);

        B[i] = value + (r3 & 511) + (r4 >> 5);
    }
#pragma capc profitability_region end

    printf("%d %d\n", B[0], B[N - 1]);

    return 0;
}