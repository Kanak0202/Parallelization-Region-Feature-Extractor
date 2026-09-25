#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <openacc.h>

/* ============================================================
 * Original source support code (original main removed)
 * ============================================================ */
#include <stdio.h>

#define N 9000

double A[N][N];
double B[N][N];
double C[N][N];

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 1; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    int i, j, k;
    
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                A[i][j] = (double)((i + j) % 17) + 1.0;
                B[i][j] = (double)((i * 2 + j) % 13) + 1.0;
            }
        }

    /* === GPU/OpenACC Runtime Initialization === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    acc_init(acc_device_nvidia);
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Device allocation only (no data movement) === */
    #pragma acc enter data create(A[0:N], B[0:N], C[0:N])
    #pragma acc wait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update device(A[0:N], B[0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 1 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma acc parallel loop collapse(2) present(A[0:N], B[0:N], C[0:N])
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                double sum = 0.0;
    
                for (k = 0; k < N; k++)
                {
                    sum += A[i][k] * B[k][j];
                }
    
                C[i][j] = sum;
            }
        }
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Required Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update self(C[0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_out = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    double __capc_t_total = __capc_t_init + __capc_t_in + __capc_t_gpu + __capc_t_out;
    printf("Region 1 Execution Breakdown:\n");
    printf("  - GPU Initialization : %f seconds\n", __capc_t_init);
    printf("  - Transfer In  (H2D): %f seconds\n", __capc_t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", __capc_t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", __capc_t_out);
    printf("  - Isolated Region Time: %f seconds\n", __capc_t_total);

    #pragma acc exit data delete(A[0:N], B[0:N], C[0:N])
    #pragma acc wait

    /* Runtime shutdown is cleanup and is intentionally not part of isolated time. */
    acc_shutdown(acc_device_nvidia);

    return 0;
}
