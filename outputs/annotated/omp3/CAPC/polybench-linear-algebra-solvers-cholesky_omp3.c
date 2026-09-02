#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[2]={0}; static unsigned long long __capc_rc[2]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
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


#define N 2000

double A[N][N];
double B[N][N];

void init_array()
{
    int i, j, r, s, t;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j <= i; j++)
            A[i][j] = (double)(-j % N) / N + 1.0;

        for (j = i + 1; j < N; j++)
            A[i][j] = 0.0;

        A[i][i] = 1.0;
    }

    /* Make A positive semi-definite. */
    for (r = 0; r < N; r++)
        for (s = 0; s < N; s++)
            B[r][s] = 0.0;

    for (t = 0; t < N; t++)
        for (r = 0; r < N; r++)
            for (s = 0; s < N; s++)
                B[r][s] += A[r][t] * A[s][t];

    for (r = 0; r < N; r++)
        for (s = 0; s < N; s++)
            A[r][s] = B[r][s];
}

void kernel_cholesky()
{
    int i, j, k;

    for (i = 0; i < N; i++)
    {
        /* j < i */
        for (j = 0; j < i; j++)
        {
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
#pragma omp parallel for private(k)
            for (k = 0; k < j; k++)
            {
#pragma omp atomic update
                A[i][j] -= A[i][k] * A[j][k];
            }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

            A[i][j] /= A[j][j];
        }

        /* i == j */
#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
#pragma omp parallel for private(k)
        for (k = 0; k < i; k++)
        {
#pragma omp atomic update
            A[i][i] -= A[i][k] * A[i][k];
        }
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

        A[i][i] = sqrt(A[i][i]);
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_cholesky();

    /* prevent complete dead-code elimination */
    printf("%f\n", A[N - 1][N - 1]);

    return 0;
}