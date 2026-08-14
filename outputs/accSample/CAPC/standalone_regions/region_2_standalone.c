#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>

//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>

#define N 8000


int main()
{

    int i, j, k, t;
    struct timespec t_start, t_end;
    double t_in = 0.0, t_gpu = 0.0, t_out = 0.0;

    /* === STAGE 1 & 2: Interleaved Setup & Prerequisite Regions === */
    double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];

    #pragma acc enter data create(a[0:N][0:N],b[0:N][0:N],c[0:N][0:N],d[0:N][0:N],e[0:N][0:N],f[0:N][0:N],result[0:N][0:N])

	//Array Initialization

    // Dependent Region 1
        #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a,b,c,d,e,f,result)
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
    #pragma capc profitability_region end
    #pragma acc wait

    //result = a.b

    /* === Pre-timing Copyin skipped: Region is write-only / copyout or has prior dependencies === */

    /* === Isolated Kernel Timing for Target Region 2 === */
    clock_gettime(CLOCK_MONOTONIC, &t_start);

        #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(a,b,result)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    t_gpu = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    /* === Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    #pragma acc update self(a[0:N][0:N], b[0:N][0:N], result[0:N][0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    t_out = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    /* === STAGE 4: Reporting Breakdown === */
    double t_total = t_in + t_gpu + t_out;
    printf("Region 2 Execution Breakdown:\n");
    printf("  - Transfer In  (H2D): %f seconds\n", t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", t_out);
    printf("  - Total Region Time : %f seconds\n", t_total);

    return 0;
}