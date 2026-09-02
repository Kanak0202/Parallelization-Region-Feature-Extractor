#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[3]={0}; static unsigned long long __capc_rc[3]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
 for(q=0;q<3;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
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
double cov[M][M];
double mean[M];

void init_array()
{
    for (int i = 0; i < N; i++)
        for (int j = 0; j < M; j++)
            data[i][j] = (double)(i * j) / M;
}

void kernel_covariance()
{
    int i, j, k;
    double float_n = N;

    /*
     * Calculate the mean of each column.
     *
     * The j iterations are independent.
     * The inner i loop is a reduction into mean[j].
     */
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
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
     * Subtract the mean from every element.
     *
     * Every (i,j) iteration modifies a different
     * data[i][j], so the iterations are independent.
     */
#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
    for (i = 0; i < N; i++)
        for (j = 0; j < M; j++)
            data[i][j] -= mean[j];
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

    /*
     * Calculate the covariance matrix.
     *
     * Each (i,j) pair calculates one covariance value.
     * The k loop is a reduction into cov[i][j].
     *
     * For every pair, cov[i][j] and cov[j][i] are
     * written together, and different (i,j) pairs do
     * not write the same matrix elements.
     */
#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
    for (i = 0; i < M; i++)
    {
        for (j = i; j < M; j++)
        {
            cov[i][j] = 0.0;

            for (k = 0; k < N; k++)
                cov[i][j] += data[k][i] * data[k][j];

            cov[i][j] /= (float_n - 1.0);
            cov[j][i] = cov[i][j];
        }
    }
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_covariance();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", cov[0][0]);

    return 0;
}