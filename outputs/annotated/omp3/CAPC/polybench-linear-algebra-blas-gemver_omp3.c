#include <stdio.h>
#include <stdlib.h>
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


#define N 2000

double A[N][N];
double u1[N];
double v1[N];
double u2[N];
double v2[N];
double w[N];
double x[N];
double y[N];
double z[N];

void init_array()
{
    double fn = (double)N;

    for (int i = 0; i < N; i++)
    {
        u1[i] = i;
        u2[i] = ((i + 1) / fn) / 2.0;
        v1[i] = ((i + 1) / fn) / 4.0;
        v2[i] = ((i + 1) / fn) / 6.0;
        y[i] = ((i + 1) / fn) / 8.0;
        z[i] = ((i + 1) / fn) / 9.0;

        x[i] = 0.0;
        w[i] = 0.0;

        for (int j = 0; j < N; j++)
            A[i][j] = (double)(i * j % N) / N;
    }
}

void kernel_gemver()
{
    int i, j;

    double alpha = 1.5;
    double beta = 1.2;

    /*
     * A = A + u1*v1^T + u2*v2^T
     *
     * Every A[i][j] is independent.
     */
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
#pragma omp parallel for private(j)
        for (j = 0; j < N; j++)
            A[i][j] =
                A[i][j]
                + u1[i] * v1[j]
                + u2[i] * v2[j];
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

    /*
     * x = x + beta * A^T * y
     *
     * Different i values update different x[i].
     * The j loop is a reduction into x[i].
     */
#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            x[i] += beta * A[j][i] * y[j];
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

    /*
     * x = x + z
     *
     * Each x[i] is independent.
     */
#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
#pragma omp parallel for private(i)
    for (i = 0; i < N; i++)
        x[i] += z[i];
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end

    /*
     * w = w + alpha * A * x
     *
     * Different i values update different w[i].
     * The j loop is a reduction into w[i].
     */
#pragma capc profitability_region begin
double __capc_rs_3=omp_get_wtime();
#pragma omp parallel for private(i,j)
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            w[i] += alpha * A[i][j] * x[j];
__capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
__capc_rc[3]++;
#pragma capc profitability_region end
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_gemver();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", w[0]);

    return 0;
}