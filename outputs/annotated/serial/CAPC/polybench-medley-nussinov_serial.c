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


#define N 2500

typedef char base;

base seq[N];
int table[N][N];

#define match(b1, b2) (((b1) + (b2)) == 3 ? 1 : 0)
#define max_score(s1, s2) ((s1) >= (s2) ? (s1) : (s2))

void init_array()
{
    int i, j;

    for (i = 0; i < N; i++)
        seq[i] = (base)((i + 1) % 4);

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
            table[i][j] = 0;
}

void kernel_nussinov()
{
    int i, j, k;

    for (i = N - 1; i >= 0; i--)
    {
        for (j = i + 1; j < N; j++)
        {
            if (j - 1 >= 0)
                table[i][j] =
                    max_score(table[i][j],
                              table[i][j - 1]);

            if (i + 1 < N)
                table[i][j] =
                    max_score(table[i][j],
                              table[i + 1][j]);

            if (j - 1 >= 0 && i + 1 < N)
            {
                if (i < j - 1)
                    table[i][j] =
                        max_score(
                            table[i][j],
                            table[i + 1][j - 1]
                            + match(seq[i], seq[j])
                        );
                else
                    table[i][j] =
                        max_score(
                            table[i][j],
                            table[i + 1][j - 1]
                        );
            }

#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
            for (k = i + 1; k < j; k++)
            {
                table[i][j] =
                    max_score(
                        table[i][j],
                        table[i][k] + table[k + 1][j]
                    );
            }
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end
        }
    }
}

int main()
{
    atexit(__capc_report);
    init_array();
    kernel_nussinov();

    /* Prevent complete dead-code elimination. */
    printf("%d\n", table[0][N - 1]);

    return 0;
}