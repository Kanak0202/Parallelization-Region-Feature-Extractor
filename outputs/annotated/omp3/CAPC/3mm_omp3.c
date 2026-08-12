//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>
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


#define N 5000

int main()
{
    atexit(__capc_report);
	int i,j,k;
	double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];

	//Array Initialization
    #pragma capc profitability_region begin
    double __capc_rs_0=omp_get_wtime();
    #pragma omp parallel for collapse(2) private(i,j)
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			a[i][j]=(double)(0.1*i+j);	
			b[i][j]=(double)(0.2*j+i);
			c[i][j]=(double)(0.3*i+j);
			d[i][j]=(double)(0.4*j+i);
			e[i][j]=(double)(0.5*i+j);
			f[i][j]=(double)(0.6*j+i);
			result[i][j]=0.0; printf("");
		}
	}
    __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
    __capc_rc[0]++;
    #pragma capc profitability_region end

	//result = a.b
    #pragma capc profitability_region begin
    double __capc_rs_1=omp_get_wtime();
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];
    __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
    __capc_rc[1]++;
    #pragma capc profitability_region end

	//print a.b
	
       printf("A[0][0]=%lf\n",result[0][0]);
       printf("A[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);
#if 0
	printf("\nResult=A.B\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}

#endif
#if 1
	//result = c.d
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+c[i][k]*d[k][j];
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

	//print c.d

       printf("B[0][0]=%lf\n",result[0][0]);
       printf("B[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);

#if 0

	printf("\nResult=C.D\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}
#endif
	//result = e.f
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+e[i][k]*f[k][j];
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end

	//print e.f
       printf("C[0][0]=%lf\n",result[0][0]);
       printf("C[%d][%d]=%lf\n",N-1,N-1,result[N-1][N-1]);
   
 #endif

#if 0	
	printf("\nResult=E.F\n");
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",result[i][j]);
		}
		printf("\n");
	}
#endif
	return 0;
}