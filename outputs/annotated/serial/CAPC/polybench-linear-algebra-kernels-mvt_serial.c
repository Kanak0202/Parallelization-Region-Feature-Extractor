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


#define N 2000

double A[N][N];

double x1[N];
double x2[N];
double y_1[N];
double y_2[N];

void init_array()
{
    for (int i = 0; i < N; i++)
    {
        x1[i] = (double)(i % N) / N;
        x2[i] = (double)((i + 1) % N) / N;
        y_1[i] = (double)((i + 3) % N) / N;
        y_2[i] = (double)((i + 4) % N) / N;

        for (int j = 0; j < N; j++)
            A[i][j] = (double)(i * j % N) / N;
    }
}

void kernel_mvt()
{
    int i, j;

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            x1[i] = x1[i] + A[i][j] * y_1[j];
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
            x2[i] = x2[i] + A[j][i] * y_2[j];
    }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_mvt();

    /* prevent complete dead-code elimination */
    printf("%f %f\n", x1[0], x2[0]);

    return 0;
}