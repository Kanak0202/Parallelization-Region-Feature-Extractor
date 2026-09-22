
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


#define N 512

int main()
{
    atexit(__capc_report);
    static int A[N][N];
    static int B[N][N];
    static int C[N][N];
    static int D[N][N];

    int i, j;

    // Initialize matrix
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] = (i * 3 + j * 7) % 1000;
        }
    }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    // Transpose matrix
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            B[j][i] = A[i][j];
        }
    }
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    // Conditional normalization
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            int value = B[i][j];

            if (value > 750)
                C[i][j] = value / 5;
            else if (value > 300)
                C[i][j] = value / 3;
            else
                C[i][j] = value / 2;
        }
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // Independent matrix transformation
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if ((i + j) % 2 == 0)
                D[i][j] = C[i][j] + 10;
            else
                D[i][j] = C[i][j] - 10;
        }
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

    printf("D[0][0] = %d\n", D[0][0]);

    return 0;
}