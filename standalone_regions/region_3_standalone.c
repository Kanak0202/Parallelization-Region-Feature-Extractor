#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <openacc.h>

/* ============================================================
 * Original source support code (original main removed)
 * ============================================================ */
//3 Matrix Multiplications (E=A.B; F=C.D; G=E.F)

#include<stdio.h>

#define N 14

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 3; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    int i,j,k;
    	double a[N][N],b[N][N],c[N][N],d[N][N],e[N][N],f[N][N],result[N][N];
    
    	//Array Initialization
    /* Earlier CAPC producer/initializer replayed on host. */
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
    			result[i][j]=0.0;
    		}
    	}
    /* Earlier CAPC compute region omitted from standalone host replay. */

    /* === Synthetic initialization for inputs whose prior expensive CAPC producer was omitted === */
    /* Synthetic valid input for 'result': prior CAPC producer was skipped. */
    for (size_t __capc_z0_0 = 0; __capc_z0_0 < (size_t)(N); ++__capc_z0_0) {
        for (size_t __capc_z0_1 = 0; __capc_z0_1 < (size_t)(N); ++__capc_z0_1) {
            result[(0) + __capc_z0_0][(0) + __capc_z0_1] = 0;
        }
    }

    /* === GPU/OpenACC Runtime Initialization === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    acc_init(acc_device_nvidia);
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Device allocation only (no data movement) === */
    #pragma acc enter data create(c[0:N][0:N], d[0:N][0:N], result[0:N][0:N])
    #pragma acc wait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update device(c[0:N][0:N], d[0:N][0:N], result[0:N][0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 3 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(c[0:N][0:N], d[0:N][0:N], result[0:N][0:N])
    	for (i = 0; i < N; i++)
    		for (j = 0; j < N; j++)
    			for (k = 0; k < N; k++)
    				result[i][j]= result[i][j]+c[i][k]*d[k][j];
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Required Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update self(result[0:N][0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_out = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    double __capc_t_total = __capc_t_init + __capc_t_in + __capc_t_gpu + __capc_t_out;
    printf("Region 3 Execution Breakdown:\n");
    printf("  - GPU Initialization : %f seconds\n", __capc_t_init);
    printf("  - Transfer In  (H2D): %f seconds\n", __capc_t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", __capc_t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", __capc_t_out);
    printf("  - Isolated Region Time: %f seconds\n", __capc_t_total);

    #pragma acc exit data delete(c[0:N][0:N], d[0:N][0:N], result[0:N][0:N])
    #pragma acc wait

    /* Runtime shutdown is cleanup and is intentionally not part of isolated time. */
    acc_shutdown(acc_device_nvidia);

    return 0;
}
