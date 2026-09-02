#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[1]={0}; static unsigned long long __capc_rc[1]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
 for(q=0;q<1;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


#define NR 150
#define NQ 140
#define NP 160

double A[NR][NQ][NP];
double C4[NP][NP];
double sum[NP];

void init_array()
{
    for (int i = 0; i < NR; i++)
        for (int j = 0; j < NQ; j++)
            for (int k = 0; k < NP; k++)
                A[i][j][k] = (double)((i * j + k) % NP) / NP;

    for (int i = 0; i < NP; i++)
        for (int j = 0; j < NP; j++)
            C4[i][j] = (double)(i * j % NP) / NP;
}

void kernel_doitgen()
{
    int r, q, p, s;

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(r,q,p,s,sum)
    for (r = 0; r < NR; r++)
    {
#pragma omp parallel for private(q,p,s,sum)
        for (q = 0; q < NQ; q++)
        {
            for (p = 0; p < NP; p++)
            {
                sum[p] = 0.0;

                for (s = 0; s < NP; s++)
                    sum[p] += A[r][q][s] * C4[s][p];
            }

            for (p = 0; p < NP; p++)
                A[r][q][p] = sum[p];
        }
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_doitgen();

    /* prevent complete dead-code elimination */
    printf("%f\n", A[0][0][0]);

    return 0;
}