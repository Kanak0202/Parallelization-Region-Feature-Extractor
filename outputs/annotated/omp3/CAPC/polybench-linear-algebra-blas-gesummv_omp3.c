#include <stdio.h>
#include <stdlib.h>
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


#define N 1300

double A[N][N];
double B[N][N];
double tmp[N];
double x[N];
double y[N];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < N; i++)
    {
        x[i] = (double)(i % N) / N;

        for (int j = 0; j < N; j++)
        {
            A[i][j] = (double)((i * j + 1) % N) / N;
            B[i][j] = (double)((i * j + 2) % N) / N;
        }
    }
}

void kernel_gesummv()
{
    int i, j;

    double alpha = 1.5;
    double beta = 1.2;

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
    {
        tmp[i] = 0.0;
        y[i] = 0.0;

        for (j = 0; j < N; j++)
        {
            tmp[i] = A[i][j] * x[j] + tmp[i];
            y[i] = B[i][j] * x[j] + y[i];
        }

        y[i] = alpha * tmp[i] + beta * y[i];
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_gesummv();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", y[0]);

    return 0;
}