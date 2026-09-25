
#include <stdio.h>
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


#define N 1000000

int main()
{
    atexit(__capc_report);
    static int a[N], b[N], normalized[N], category[N];
    int i;

    // Initialize arrays
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        a[i] = (i * 17 + 13) % 1000;
        b[i] = (i * 23 + 7) % 1000;
    }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    // Integer division and conditional processing
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        int value = a[i] + b[i];

        if (value > 1200)
            normalized[i] = value / 4;
        else if (value > 600)
            normalized[i] = value / 3;
        else
            normalized[i] = value / 2;
    }
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    // Classification
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        if (normalized[i] > 300)
            category[i] = 3;
        else if (normalized[i] > 150)
            category[i] = 2;
        else if (normalized[i] > 50)
            category[i] = 1;
        else
            category[i] = 0;
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // Independent transformation
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        b[i] = (normalized[i] * 7 + category[i]) / 2;
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

    printf("Normalized[%d] = %d\n", N, normalized[N]);
    printf("Category[%d] = %d\n", N, category[N]);

    return 0;
}