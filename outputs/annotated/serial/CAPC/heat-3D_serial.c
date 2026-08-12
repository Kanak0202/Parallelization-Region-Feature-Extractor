#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
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

#define n 10

int main()
{
    atexit(__capc_report);
	int i, j, k;

	float A[n][n][n],B[n][n][n];
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				A[i][j][k] = B[i][j][k] = (float) (i + j + (n-k))* 10 / (n);
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				B[i][j][k] = 0.125 * (A[i+1][j][k] - (2.0) * A[i][j][k] + A[i-1][j][k])
					+ 0.125 * (A[i][j+1][k] - (2.0) * A[i][j][k] + A[i][j-1][k])
					+ 0.125 * (A[i][j][k+1] -(2.0) * A[i][j][k] + A[i][j][k-1])
					+ A[i][j][k];
			}
		}
	}
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				A[i][j][k] = 0.125 * (B[i+1][j][k] - (2.0) * B[i][j][k] + B[i-1][j][k])
					+ 0.125 * (B[i][j+1][k] - (2.0) * B[i][j][k] + B[i][j-1][k])
					+ 0.125 * (B[i][j][k+1] - (2.0) * B[i][j][k] + B[i][j][k-1])
					+ B[i][j][k];
			}
		}
	}
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f",A[i][j][k]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f ",B[i][j][k]);

	printf("\n");

	return 0;
}

