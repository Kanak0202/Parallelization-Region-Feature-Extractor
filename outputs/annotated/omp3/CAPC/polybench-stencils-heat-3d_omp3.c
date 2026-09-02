#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[2]={0}; static unsigned long long __capc_rc[2]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
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


#define N 120
#define TSTEPS 500

double A[N][N][N];
double B[N][N][N];

void init_array()
{
    int i, j, k;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            for (k = 0; k < N; k++)
            {
                A[i][j][k] =
                B[i][j][k] =
                    (double)(i + j + (N - k)) * 10.0 / N;
            }
        }
    }
}

void kernel_heat_3d()
{
    int t, i, j, k;

    for (t = 1; t <= TSTEPS; t++)
    {
        /* -------------------------------------------------- */
        /* Region 0: A -> B stencil                           */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(j,k)
        for (i = 1; i < N - 1; i++)
        {
#pragma omp parallel for private(k)
            for (j = 1; j < N - 1; j++)
            {
#pragma omp parallel for
                for (k = 1; k < N - 1; k++)
                {
                    B[i][j][k] =
                          0.125 *
                          (A[i + 1][j][k]
                           - 2.0 * A[i][j][k]
                           + A[i - 1][j][k])

                        + 0.125 *
                          (A[i][j + 1][k]
                           - 2.0 * A[i][j][k]
                           + A[i][j - 1][k])

                        + 0.125 *
                          (A[i][j][k + 1]
                           - 2.0 * A[i][j][k]
                           + A[i][j][k - 1])

                        + A[i][j][k];
                }
            }
        }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: B -> A stencil                           */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(j,k)
        for (i = 1; i < N - 1; i++)
        {
#pragma omp parallel for private(k)
            for (j = 1; j < N - 1; j++)
            {
#pragma omp parallel for
                for (k = 1; k < N - 1; k++)
                {
                    A[i][j][k] =
                          0.125 *
                          (B[i + 1][j][k]
                           - 2.0 * B[i][j][k]
                           + B[i - 1][j][k])

                        + 0.125 *
                          (B[i][j + 1][k]
                           - 2.0 * B[i][j][k]
                           + B[i][j - 1][k])

                        + 0.125 *
                          (B[i][j][k + 1]
                           - 2.0 * B[i][j][k]
                           + B[i][j][k - 1])

                        + B[i][j][k];
                }
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
    kernel_heat_3d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2][N / 2][N / 2]);

    return 0;
}