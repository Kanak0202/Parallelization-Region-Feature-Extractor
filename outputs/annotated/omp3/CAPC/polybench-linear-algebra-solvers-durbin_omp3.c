#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[3]={0}; static unsigned long long __capc_rc[3]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
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


#define N 2000

double r[N];
double y[N];
double z[N];

void init_array()
{
    for (int i = 0; i < N; i++)
        r[i] = (double)(N + 1 - i);
}

void kernel_durbin()
{
    int i, k;
    double alpha;
    double beta;
    double sum;


    y[0] = -r[0];
    beta = 1.0;
    alpha = -r[0];

    for (k = 1; k < N; k++)
    {
        beta = (1.0 - alpha * alpha) * beta;

        sum = 0.0;
        #pragma capc profitability_region begin
        double __capc_rs_0=omp_get_wtime();
        #pragma omp parallel for private(i) reduction(+:sum)
        for (i = 0; i < k; i++)
            sum += r[k - i - 1] * y[i];
        __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
        __capc_rc[0]++;
        #pragma capc profitability_region end

        alpha = -(r[k] + sum) / beta;

        #pragma capc profitability_region begin
        double __capc_rs_1=omp_get_wtime();
        #pragma omp parallel for private(i)
        for (i = 0; i < k; i++)
            z[i] = y[i] + alpha * y[k - i - 1];
        __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
        __capc_rc[1]++;
        #pragma capc profitability_region end

        #pragma capc profitability_region begin
        double __capc_rs_2=omp_get_wtime();
        #pragma omp parallel for private(i)
        for (i = 0; i < k; i++)
            y[i] = z[i];
        __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
        __capc_rc[2]++;
        #pragma capc profitability_region end

        y[k] = alpha;
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_durbin();

    /* prevent complete dead-code elimination */
    printf("%f\n", y[N - 1]);

    return 0;
}