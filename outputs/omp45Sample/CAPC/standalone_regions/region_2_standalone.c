#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <omp.h>

//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>

#define N 8000


int main()
{

    int i, j, k, t;
    struct timespec start_h2d, end_h2d, start_exec, end_exec, start_d2h, end_d2h;
    double t_h2d = 0.0, t_exec = 0.0, t_d2h = 0.0;

    /* === STAGE 1 & 2: Interleaved Setup & Prerequisite Regions === */
    double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];

    #pragma omp target enter data map(alloc:a[0:N][0:N],b[0:N][0:N],c[0:N][0:N],d[0:N][0:N],e[0:N][0:N],f[0:N][0:N],result[0:N][0:N])

	//Array Initialization

    // Dependent Region 1
        #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) private(i,j)
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

    //result = a.b

    /* === STAGE 3a: Pre-timing H2D Copyin skipped (write-only or has prior dependencies) === */

    /* === STAGE 3b: Isolated Execution for Target Region 2 === */
    clock_gettime(CLOCK_MONOTONIC, &start_exec);

        #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) private(i,j,k)
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				result[i][j]= result[i][j]+a[i][k]*b[k][j];
    #pragma capc profitability_region end

    clock_gettime(CLOCK_MONOTONIC, &end_exec);
    t_exec = (end_exec.tv_sec - start_exec.tv_sec) + (end_exec.tv_nsec - start_exec.tv_nsec) / 1e9;

    /* === STAGE 3c: Timed Device -> Host (D2H) Data Transfer === */
    clock_gettime(CLOCK_MONOTONIC, &start_d2h);
    #pragma omp target update from(result[0:N][0:N], a[0:N][0:N], b[0:N][0:N])
    clock_gettime(CLOCK_MONOTONIC, &end_d2h);
    t_d2h = (end_d2h.tv_sec - start_d2h.tv_sec) + (end_d2h.tv_nsec - start_d2h.tv_nsec) / 1e9;

    /* === STAGE 4: Performance Profile Breakdown === */
    double t_transfer = t_h2d + t_d2h;
    double t_total = t_exec + t_transfer;
    printf("Region 2 Performance Breakdown:\n");
    printf("  - H2D Transfer Time : %f seconds\n", t_h2d);
    printf("  - Kernel Execution  : %f seconds\n", t_exec);
    printf("  - D2H Transfer Time : %f seconds\n", t_d2h);
    printf("  - Total Transfer    : %f seconds\n", t_transfer);
    printf("  - Total Region Time : %f seconds\n\n", t_total);
    return 0;
}