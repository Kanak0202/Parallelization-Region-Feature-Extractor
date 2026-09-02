#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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


#define M 1200
#define N 1400

double data[N][M];
double corr[M][M];
double mean[M];
double stddev[M];

void init_array()
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            data[i][j] = (double)(i * j) / M + i;
}

void kernel_correlation()
{
    int i, j, k;
    double float_n = N;
    double eps = 0.1;

    /*
     * Calculate the mean of each column.
     */
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(j,i)
    for (j = 0; j < M; j++)
    {
        mean[j] = 0.0;

        for (i = 0; i < N; i++)
            mean[j] += data[i][j];

        mean[j] /= float_n;
    }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

    /*
     * Calculate the standard deviation of each column.
     */
#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(j,i)
    for (j = 0; j < M; j++)
    {
        stddev[j] = 0.0;

        for (i = 0; i < N; i++)
            stddev[j] +=
                (data[i][j] - mean[j]) *
                (data[i][j] - mean[j]);

        stddev[j] /= float_n;
        stddev[j] = sqrt(stddev[j]);

        stddev[j] = stddev[j] <= eps ? 1.0 : stddev[j];
    }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

    /*
     * Center and reduce the column vectors.
     */
#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
#pragma omp parallel for private(j)
        for (j = 0; j < M; j++)
        {
            data[i][j] -= mean[j];
            data[i][j] /= sqrt(float_n) * stddev[j];
        }
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end

    /*
     * Calculate the M x M correlation matrix.
     */
#pragma capc profitability_region begin
double __capc_rs_3=omp_get_wtime();
#pragma omp parallel for private(i,j,k)
    for (i = 0; i < M - 1; i++)
    {
        corr[i][i] = 1.0;

#pragma omp parallel for private(j,k)
        for (j = i + 1; j < M; j++)
        {
            corr[i][j] = 0.0;

            for (k = 0; k < N; k++)
                corr[i][j] += data[k][i] * data[k][j];

            corr[j][i] = corr[i][j];
        }
    }

    corr[M - 1][M - 1] = 1.0;
__capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
__capc_rc[3]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_correlation();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", corr[0][0]);

    return 0;
}