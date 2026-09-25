#include <stdio.h>
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


#define N 10000000

double A[N];
double B[N];

int main()
{
    atexit(__capc_report);
    int i;

    for (i = 0; i < N; i++)
        A[i] = (double)(i + 1);

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
    for (i = 0; i < N - 16; i++)
    {
        B[i] = A[i]
             + A[i + 2]
             + A[i + 4]
             + A[i + 8]
             + A[i + 16];
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

    printf("%f %f\n", B[0], B[N - 17]);

    return 0;
}