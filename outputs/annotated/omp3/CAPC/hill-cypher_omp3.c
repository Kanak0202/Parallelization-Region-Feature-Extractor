// Hill Cipher - OpenMP 3.0 Version
// No user/file input
// Message and key matrix are initialized internally

#include <stdio.h>
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


#define N 100

int main()
{
    atexit(__capc_report);
	int i, j, k;

	float encrypt[N][1];
	float a[N][N];
	float mes[N][1];

	/* ---------------------------------------------------------
	   Message Initialization
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	double __capc_rs_0=omp_get_wtime();
	#pragma omp parallel for private(i)
	for(i = 0; i < N; i++)
	{
		mes[i][0] = (float)(i % 26);
		encrypt[i][0] = 0.0f;
	}
	__capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
	__capc_rc[0]++;
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Key Matrix Initialization
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	double __capc_rs_1=omp_get_wtime();
	#pragma omp parallel for private(i,j)
	for(i = 0; i < N; i++)
	{
		#pragma omp parallel for private(j)
		for(j = 0; j < N; j++)
		{
			a[i][j] = (float)(i + j + 1 + '0');
		}
	}
	__capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
	__capc_rc[1]++;
	#pragma capc profitability_region end


	/* ---------------------------------------------------------
	   Hill Cipher Encryption

	   encrypt = a * mes
	   --------------------------------------------------------- */

	#pragma capc profitability_region begin
	double __capc_rs_2=omp_get_wtime();
	#pragma omp parallel for private(i,j,k)
	for(i = 0; i < N; i++)
	{
		#pragma omp parallel for private(j,k)
		for(j = 0; j < 1; j++)
		{
			for(k = 0; k < N; k++)
			{
				encrypt[i][j] =
					encrypt[i][j] +
					a[i][k] * mes[k][j];
			}
		}
	}
	__capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
	__capc_rc[2]++;
	#pragma capc profitability_region end


	printf("encrypt[0][0] = %f\n", encrypt[0][0]);
	printf("encrypt[%d][0] = %f\n", N-1, encrypt[N-1][0]);

	printf("Encrypted characters: ");
	printf("%c ", (char)(fmod(encrypt[0][0], 26.0f) + 97));
	printf("%c\n", (char)(fmod(encrypt[N-1][0], 26.0f) + 97));

	return 0;
}