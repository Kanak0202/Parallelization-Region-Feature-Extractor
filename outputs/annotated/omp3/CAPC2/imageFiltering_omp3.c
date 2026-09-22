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
    static int image[N][N];
    static int filtered[N][N];
    static int enhanced[N][N];
    static int output[N][N];

    int i, j;

    // Initialize image
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            image[i][j] = (i * 11 + j * 13) % 256;
        }
    }
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    // 3x3 average filter
    // Boundary pixels are copied independently.
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (i == 0 || j == 0 ||
                i == N - 1 || j == N - 1)
            {
                filtered[i][j] = image[i][j];
            }
            else
            {
                filtered[i][j] =
                    (image[i-1][j-1] + image[i-1][j] +
                     image[i-1][j+1] + image[i][j-1] +
                     image[i][j] + image[i][j+1] +
                     image[i+1][j-1] + image[i+1][j] +
                     image[i+1][j+1]) / 9;
            }
        }
    }
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    // Enhance image
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (filtered[i][j] > 180)
                enhanced[i][j] = filtered[i][j] / 2;
            else if (filtered[i][j] > 100)
                enhanced[i][j] = filtered[i][j] * 2;
            else
                enhanced[i][j] = filtered[i][j] + 20;
        }
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // Threshold classification
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for private(j)
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (enhanced[i][j] > 200)
                output[i][j] = 255;
            else if (enhanced[i][j] > 100)
                output[i][j] = 128;
            else
                output[i][j] = 0;
        }
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

    printf("Output[100][100] = %d\n", output[100][100]);

    return 0;
}