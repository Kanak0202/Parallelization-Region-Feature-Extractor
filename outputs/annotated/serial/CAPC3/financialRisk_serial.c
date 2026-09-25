
#include <stdio.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[5]={0}; static unsigned long long __capc_rc[5]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
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


#define N 512

int main()
{
    atexit(__capc_report);
    static int assets[N][N];
    static int risk[N][N];
    static int normalized[N][N];
    static int category[N][N];

    int i, j;

    // Initialize asset data
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            assets[i][j] = (i * 19 + j * 7) % 1000;
        }
    }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    // Calculate risk scores
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            int value = assets[i][j];

            if (value > 700)
                risk[i][j] = value * 3 + 100;
            else if (value > 300)
                risk[i][j] = value * 2 + 50;
            else
                risk[i][j] = value + 20;
        }
    }
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    // Normalize risk scores
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (risk[i][j] > 2000)
                normalized[i][j] = risk[i][j] / 5;
            else if (risk[i][j] > 1000)
                normalized[i][j] = risk[i][j] / 3;
            else
                normalized[i][j] = risk[i][j] / 2;
        }
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // Risk classification
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (normalized[i][j] > 500)
                category[i][j] = 3;
            else if (normalized[i][j] > 250)
                category[i][j] = 2;
            else if (normalized[i][j] > 100)
                category[i][j] = 1;
            else
                category[i][j] = 0;
        }
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

    // Final independent adjustment
    #pragma capc profitability_region begin
    double __capc_rs_4=omp_get_wtime();
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            risk[i][j] = normalized[i][j] + category[i][j] * 10;
        }
    }
    __capc_rt[4]+=omp_get_wtime()-__capc_rs_4;
    __capc_rc[4]++;
    #pragma capc profitability_region end

    printf("Risk[%d][%d] = %d\n", N, N, risk[N][N]);
    printf("Category[%d][%d] = %d\n", category[N][N]);

    return 0;
}