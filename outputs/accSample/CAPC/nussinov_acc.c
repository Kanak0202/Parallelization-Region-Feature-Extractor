#include <stdio.h>
#include <stdlib.h>

#define N 2500

typedef char base;

base seq[N];
int table[N][N];

#define match(b1, b2) (((b1) + (b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1) >= (s2) ? (s1) : (s2))

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
        seq[i] = (base)((i + 1) % 4);

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            table[i][j] = 0;
}

void kernel_nussinov()
{
    int i, j, k;

    for (i = N - 1; i >= 0; i--)
    {
        for (j = i + 1; j < N; j++)
        {
            if (j - 1 >= 0)
                table[i][j] =
                    max_score(table[i][j],
                              table[i][j - 1]);

            if (i + 1 < N)
                table[i][j] =
                    max_score(table[i][j],
                              table[i + 1][j]);

            if (j - 1 >= 0 && i + 1 < N)
            {
                if (i < j - 1)
                    table[i][j] =
                        max_score(
                            table[i][j],
                            table[i + 1][j - 1]
                            + match(seq[i], seq[j])
                        );
                else
                    table[i][j] =
                        max_score(
                            table[i][j],
                            table[i + 1][j - 1]
                        );
            }

#pragma capc profitability_region begin
            for (k = i + 1; k < j; k++)
            {
                table[i][j] =
                    max_score(
                        table[i][j],
                        table[i][k] + table[k + 1][j]
                    );
            }
#pragma capc profitability_region end
        }
    }
}

int main()
{
    init_array();
    kernel_nussinov();

    /* Prevent complete dead-code elimination. */
    printf("%d\n", table[0][N - 1]);

    return 0;
}