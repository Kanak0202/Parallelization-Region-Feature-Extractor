#include <stdio.h>
#include <stdlib.h>

#define TMAX 500
#define NX 1000
#define NY 1200

double ex[NX][NY];
double ey[NX][NY];
double hz[NX][NY];
double fict[TMAX];

void init_array()
{
    int i, j;

    for (i = 0; i < TMAX; i++)
        fict[i] = (double)i;

    for (i = 0; i < NX; i++)
    {
        for (j = 0; j < NY; j++)
        {
            ex[i][j] = ((double)i * (j + 1)) / NX;
            ey[i][j] = ((double)i * (j + 2)) / NY;
            hz[i][j] = ((double)i * (j + 3)) / NX;
        }
    }
}

void kernel_fdtd_2d()
{
    int t, i, j;

    for (t = 0; t < TMAX; t++)
    {
        /* -------------------------------------------------- */
        /* Region 0: Update ey boundary                       */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin

        for (j = 0; j < NY; j++)
        {
            ey[0][j] = fict[t];
        }

#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: Update ey field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin

        for (i = 1; i < NX; i++)
        {
            for (j = 0; j < NY; j++)
            {
                ey[i][j] =
                    ey[i][j]
                    - 0.5 * (hz[i][j] - hz[i - 1][j]);
            }
        }

#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 2: Update ex field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin

        for (i = 0; i < NX; i++)
        {
            for (j = 1; j < NY; j++)
            {
                ex[i][j] =
                    ex[i][j]
                    - 0.5 * (hz[i][j] - hz[i][j - 1]);
            }
        }

#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 3: Update hz field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin

        for (i = 0; i < NX - 1; i++)
        {
            for (j = 0; j < NY - 1; j++)
            {
                hz[i][j] =
                    hz[i][j]
                    - 0.7 *
                      (ex[i][j + 1]
                       - ex[i][j]
                       + ey[i + 1][j]
                       - ey[i][j]);
            }
        }

#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_fdtd_2d();

    /* Prevent complete dead-code elimination. */
    printf("%f %f %f\n",
           ex[NX - 1][NY - 1],
           ey[NX - 1][NY - 1],
           hz[0][0]);

    return 0;
}