#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[2]={0}; static unsigned long long __capc_rc[2]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
 for(q=0;q<2;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


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
double __capc_rs_0=omp_get_wtime();

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

__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: Row sweep                                */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();

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

__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_adi();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", u[N - 1][N - 1]);

    return 0;
}