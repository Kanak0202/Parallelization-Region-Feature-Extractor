//Vector Arithmatic

#include <stdio.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[4]={0}; static unsigned long long __capc_rc[4]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (serial) =====\n");
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

#define SIZE 5

int main()
{
    atexit(__capc_report);
	double A[SIZE],B[SIZE],C[SIZE],D[SIZE],E[SIZE];
	int i = 0;

	//Array initialization
#pragma capc profitability_region begin
double __capc_rs_0=omp_get_wtime();
	for (i=0; i<SIZE; ++i)
	{
		A[i] = (double)i;
		B[i] = (double)(i+1);
	}
__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
__capc_rc[0]++;
#pragma capc profitability_region end

	//C=A+B
#pragma capc profitability_region begin
double __capc_rs_1=omp_get_wtime();
	for (i=0; i<SIZE; ++i)
	{
		C[i]=A[i]+B[i];
	}
__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
__capc_rc[1]++;
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (C[i] !=A[i] + B[i])
		{
			printf("Add : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Add : Everything seems to work fine! \n");
	}

	//D=A-B	
#pragma capc profitability_region begin
double __capc_rs_2=omp_get_wtime();
	for (i=0; i<SIZE; ++i)
	{
		D[i]=A[i]-B[i];
	}
__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
__capc_rc[2]++;
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (D[i] !=A[i] - B[i])
		{
			printf("Sub : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Sub : Everything seems to work fine! \n");
	}

	//E=A*B
#pragma capc profitability_region begin
double __capc_rs_3=omp_get_wtime();
	for (i=0; i<SIZE; ++i)
	{
		E[i]=A[i]*B[i];
	}
__capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
__capc_rc[3]++;
#pragma capc profitability_region end

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (E[i] !=A[i] * B[i])
		{
			printf("Mult : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Mult : Everything seems to work fine! \n");
	}

	return 0;
}
