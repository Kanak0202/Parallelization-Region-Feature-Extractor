#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[4]={0}; static unsigned long long __capc_rc[4]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
 for(q=0;q<4;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


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
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(j)
        for (j = 0; j < NY; j++)
        {
            ey[0][j] = fict[t];
        }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 1: Update ey field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(j)
        for (i = 1; i < NX; i++)
        {
#pragma omp parallel for private(j)
            for (j = 0; j < NY; j++)
            {
                ey[i][j] =
                    ey[i][j]
                    - 0.5 * (hz[i][j] - hz[i - 1][j]);
            }
        }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 2: Update ex field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
#pragma omp parallel for private(j)
        for (i = 0; i < NX; i++)
        {
#pragma omp parallel for private(j)
            for (j = 1; j < NY; j++)
            {
                ex[i][j] =
                    ex[i][j]
                    - 0.5 * (hz[i][j] - hz[i][j - 1]);
            }
        }
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end

        /* -------------------------------------------------- */
        /* Region 3: Update hz field                          */
        /* -------------------------------------------------- */

#pragma capc profitability_region begin
double __capc_rs_3=omp_get_wtime();
#pragma omp parallel for private(j)
        for (i = 0; i < NX - 1; i++)
        {
#pragma omp parallel for private(j)
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
__capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
__capc_rc[3]++;
#pragma capc profitability_region end
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_fdtd_2d();

    /* Prevent complete dead-code elimination. */
    printf("%f %f %f\n",
           ex[NX - 1][NY - 1],
           ey[NX - 1][NY - 1],
           hz[0][0]);

    return 0;
}