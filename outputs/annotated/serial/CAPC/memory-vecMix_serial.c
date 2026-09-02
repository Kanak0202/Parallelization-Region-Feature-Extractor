#include <stdio.h>
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


#define N 10000000

int index_array[N];

double A[N];
double B[N];
double C[N];

int main()
{
    atexit(__capc_report);
    int i;

    /* ============================================================
       Region 1: Initialization
       ============================================================ */

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        index_array[i] = (i * 13 + 7) % N;

        A[i] = (double)i;
        B[i] = (double)(N - i);
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end


    /* ============================================================
       Region 2: Mixed computation
       ============================================================ */

#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        int idx = index_array[i];
        int offset = (idx * 3) + 1;

        if (offset > N / 2)
        {
            C[i] = A[i] + 2.5 * B[idx];
        }
        else
        {
            C[i] = A[i] - 1.5 * B[idx];
        }
    }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

    printf("C[0] = %f\n", C[0]);
    printf("C[N-1] = %f\n", C[N - 1]);

    return 0;
}