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
 for(q=0;q<0;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
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
#define TSTEPS 500

double A[N][N];

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            A[i][j] =
                ((double)i * (j + 2) + 2.0) / N;
        }
    }
}

void kernel_seidel_2d()
{
    int t, i, j;

    for (t = 0; t < TSTEPS; t++)
    {
        for (i = 1; i < N - 1; i++)
        {
            for (j = 1; j < N - 1; j++)
            {
                A[i][j] =
                    (A[i - 1][j - 1]
                     + A[i - 1][j]
                     + A[i - 1][j + 1]
                     + A[i][j - 1]
                     + A[i][j]
                     + A[i][j + 1]
                     + A[i + 1][j - 1]
                     + A[i + 1][j]
                     + A[i + 1][j + 1])
                    / 9.0;
            }
        }
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_seidel_2d();

    /* Prevent complete dead-code elimination. */
    printf("%f\n", A[N / 2][N / 2]);

    return 0;
}