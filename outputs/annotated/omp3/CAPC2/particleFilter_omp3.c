
#include <stdio.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[5]={0}; static unsigned long long __capc_rc[5]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
 for(q=0;q<5;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
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
    static int x[N], y[N], z[N];
    static int distance[N];
    static int normalized[N];
    static int category[N];

    int i;

    // Initialize particle coordinates
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        x[i] = (i * 17 + 11) % 10000;
        y[i] = (i * 23 + 19) % 10000;
        z[i] = (i * 31 + 29) % 10000;
    }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    // Compute a Manhattan-style distance
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        int ax = x[i];
        int ay = y[i];
        int az = z[i];

        distance[i] = ax + ay + az;
    }
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    // Normalize distance
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        if (distance[i] > 20000)
            normalized[i] = distance[i] / 4;
        else if (distance[i] > 10000)
            normalized[i] = distance[i] / 3;
        else
            normalized[i] = distance[i] / 2;
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // Classify particles
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        if (normalized[i] > 10000)
            category[i] = 3;
        else if (normalized[i] > 5000)
            category[i] = 2;
        else if (normalized[i] > 1000)
            category[i] = 1;
        else
            category[i] = 0;
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

    // Compute final particle feature
    #pragma capc profitability_region begin
    double __capc_rs_4=omp_get_wtime();
    #pragma omp parallel for
    for (i = 0; i < N; i++)
    {
        distance[i] = (normalized[i] + category[i]) / 2;
    }
    __capc_rt[4]+=omp_get_wtime()-__capc_rs_4;
    __capc_rc[4]++;
    #pragma capc profitability_region end

    printf("Distance[0] = %d\n", distance[0]);
    printf("Category[0] = %d\n", category[0]);

    return 0;
}