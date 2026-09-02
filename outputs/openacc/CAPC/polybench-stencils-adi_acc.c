#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 1000
#define TSTEPS 500

double u[N][N];
double v[N][N];
double p[N][N];
double q[N][N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            u[i][j] = (double)(i + N - j) / N;
        }
    }
}

void kernel_adi()
{
    int t, i, j;

    double DX, DY, DT;
    double B1, B2;
    double mul1, mul2;
    double a, b, c, d, e, f;

    DX = 1.0 / (double)N;
    DY = 1.0 / (double)N;
    DT = 1.0 / (double)TSTEPS;

    B1 = 2.0;
    B2 = 1.0;

    mul1 = B1 * DT / (DX * DX);
    mul2 = B2 * DT / (DY * DY);

    a = -mul1 / 2.0;
    b = 1.0 + mul1;
    c = a;

    d = -mul2 / 2.0;
    e = 1.0 + mul2;
    f = d;

    for (t = 1; t <= TSTEPS; t++)
    {
        /* -------------------------------------------------- */
        /* Region 0: Column sweep                             */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
#pragma acc parallel loop private(j) copyin(u[0:N][0:N]) copyout(v[0:N][0:N],p[0:N][0:N],q[0:N][0:N])
        for (i = 1; i < N - 1; i++)
        {
            v[0][i] = 1.0;
            p[i][0] = 0.0;
            q[i][0] = v[0][i];

            for (j = 1; j < N - 1; j++)
            {
                p[i][j] =
                    -c / (a * p[i][j - 1] + b);

                q[i][j] =
                    (-d * u[j][i - 1]
                     + (1.0 + 2.0 * d) * u[j][i]
                     - f * u[j][i + 1]
                     - a * q[i][j - 1])
                    / (a * p[i][j - 1] + b);
            }

            v[N - 1][i] = 1.0;

            for (j = N - 2; j >= 1; j--)
            {
                v[j][i] =
                    p[i][j] * v[j + 1][i]
                    + q[i][j];
            }
        }
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: Row sweep                                */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
#pragma acc parallel loop private(j) copyin(v[0:N][0:N]) copyout(u[0:N][0:N],p[0:N][0:N],q[0:N][0:N])
        for (i = 1; i < N - 1; i++)
        {
            u[i][0] = 1.0;
            p[i][0] = 0.0;
            q[i][0] = u[i][0];

            for (j = 1; j < N - 1; j++)
            {
                p[i][j] =
                    -f / (d * p[i][j - 1] + e);

                q[i][j] =
                    (-a * v[i - 1][j]
                     + (1.0 + 2.0 * a) * v[i][j]
                     - c * v[i + 1][j]
                     - d * q[i][j - 1])
                    / (d * p[i][j - 1] + e);
            }

            u[i][N - 1] = 1.0;

            for (j = N - 2; j >= 1; j--)
            {
                u[i][j] =
                    p[i][j] * u[i][j + 1]
                    + q[i][j];
            }
        }
#pragma capc profitability_region end
    }
}

int main()
{
    init_array();
    kernel_adi();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", u[N - 1][N - 1]);

    return 0;
}