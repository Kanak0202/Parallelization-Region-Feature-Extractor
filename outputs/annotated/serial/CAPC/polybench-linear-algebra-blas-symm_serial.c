#include <stdio.h>
#include <stdlib.h>
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


#define M 1000
#define N 1200

double C[M][N];
double A[M][M];
double B[M][N];

void init_array()
{
    double alpha = 1.5;
    double beta = 1.2;

    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
        {
            C[i][j] = (double)((i + j) % 100) / M;
            B[i][j] = (double)((N + i - j) % 100) / M;
        }

    for (int i = 0; i < M; i++)
    {
        for (int j = 0; j <= i; j++)
            A[i][j] = (double)((i + j) % 100) / M;

        for (int j = i + 1; j < M; j++)
            A[i][j] = -999.0;
    }
}

void kernel_symm()
{
    int i, j, k;
    double alpha = 1.5;
    double beta = 1.2;
    double temp2;

    for (i = 0; i < M; i++)
    {
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
        for (j = 0; j < N; j++)
        {
            temp2 = 0.0;

            for (k = 0; k < i; k++)
            {
                C[k][j] += alpha * B[i][j] * A[i][k];
                temp2 += B[k][j] * A[i][k];
            }

            C[i][j] = beta * C[i][j]
                    + alpha * B[i][j] * A[i][i]
                    + alpha * temp2;
        }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_symm();

    /*
     * Prevent complete dead-code elimination.
     */
    printf("%f\n", C[0][0]);

    return 0;
}
