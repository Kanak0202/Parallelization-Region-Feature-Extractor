#include <stdio.h>

#define N 10000000

int A[N];
int B[N];
int C[N];

int main()
{
    int i;

    for (i = 0; i < N; i++)
    {
        A[i] = i + 17;
        B[i] = (i % 97) + 3;
    }

#pragma capc profitability_region begin
    for (i = 0; i < N; i++)
    {
        int x = A[i];
        int y = B[i];

        if (x > 1000000)
        {
            C[i] = x / y;
        }
        else
        {
            C[i] = y / (x % 31 + 1);
        }

        C[i] = C[i] ^ (x & 255);
        C[i] = C[i] | ((y << 2) & 1023);
        C[i] = C[i] + ((x >> 3) & 127);
    }
#pragma capc profitability_region end

    printf("%d %d\n", C[0], C[N - 1]);

    return 0;
}