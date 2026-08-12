//2D Jacobi Computation

#include<stdio.h>
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

#define n 5

int main()
{
    atexit(__capc_report);
	int i,j;

	double A[n][n],B[n][n];

	//Array initialization
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			A[i][j]=(double)(0.1*i+j);
			B[i][j]=(double)(0.2*j+i);
			printf("");
		}
	}
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

	//Computations
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
	for (i = 1; i < n-1; i++)
		for (j = 1; j < n - 1; j++)
			B[i][j] = 0.2 * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);

    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
	for (i = 1; i < n - 1; i++)
		for (j = 1; j < n - 1; j++)
			A[i][j] = 0.2 * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);

    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf",A[i][j]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf ",B[i][j]);

	printf("\n");

	return 0;
}