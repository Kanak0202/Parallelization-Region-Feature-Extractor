#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[1]={0}; static unsigned long long __capc_rc[1]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
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


#define NI 1000
#define NJ 1100
#define NK 1200

double C[NI][NJ];
double A[NI][NK];
double B[NK][NJ];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NJ; j++)
            C[i][j] = (double)((i * j + 1) % NI) / NI;

    for (int i = 0; i < NI; i++)
        for (int j = 0; j < NK; j++)
            A[i][j] = (double)(i * (j + 1) % NK) / NK;

    for (int i = 0; i < NK; i++)
        for (int j = 0; j < NJ; j++)
            B[i][j] = (double)(i * (j + 2) % NJ) / NJ;
}

void kernel_gemm()
{
    int i, j, k;

    double alpha = 1.5;
    double beta = 1.2;

    /*
     * C = alpha * A * B + beta * C
     *
     * Each C[i][j] is independent of every other
     * C[i][j]. The k loop is a reduction into C[i][j].
     */
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
    for (i = 0; i < NI; i++)
    {
        for (j = 0; j < NJ; j++)
            C[i][j] *= beta;

        for (k = 0; k < NK; k++)
        {
            for (j = 0; j < NJ; j++)
                C[i][j] += alpha * A[i][k] * B[k][j];
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
    kernel_gemm();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}