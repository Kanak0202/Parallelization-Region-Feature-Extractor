#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
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


#define N 200000000
#define T 500

double a[N];
double b[N];

void init_array()
{
        int i, j;
        #pragma capc profitability_region begin
        double __capc_rs_0=omp_get_wtime();
        for (i=0; i<N; i++)
        {
                a[i] = ((double)i)/N;
                b[i] = ((double)i+1)/N;
        }
        __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
        __capc_rc[0]++;
        #pragma capc profitability_region end
}

void print_array()
{
        int i, j;

        for (i=0; i<N; i++)
                printf("%lf ", a[i]);

        printf("\n");

        for (i=0; i<N; i++)
                printf("%lf ", b[i]);
}

int main()
{
    atexit(__capc_report);
        int t, i, j;

        init_array();

        for (t = 0; t < T; t++)
        {
                
                #pragma capc profitability_region begin
                double __capc_rs_1=omp_get_wtime();
                for (i = 2; i < N - 1; i++)
                {
                        b[i] = 0.33333 * (a[i-1] + a[i] + a[i + 1]);
                }
                __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
                __capc_rc[1]++;
                #pragma capc profitability_region end
                
                
                #pragma capc profitability_region begin
                double __capc_rs_2=omp_get_wtime();
                for (i = 2; i < N - 1; i++)
                {
                        a[i] = b[i];
                }
                __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
                __capc_rc[2]++;
                #pragma capc profitability_region end
                
        }

//      print_array();

       printf("a[0]=%lf\n",a[0]);
        printf("a[%d]=%lf\n",N-2,a[N-2]);

        printf("b[0]=%lf\n",b[0]);
        printf("b[%d]=%lf\n",N-1,b[N-1]);

        return 0;
}