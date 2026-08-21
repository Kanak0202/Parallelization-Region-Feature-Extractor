#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <openacc.h>

/* ============================================================
 * Original source support code (original main removed)
 * ============================================================ */
// C program to implement Vector Arithmetic

#include <stdio.h>

#define SIZE 13000000

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 3; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    double A[13000000];
        double B[13000000];
        double C[13000000];
        double D[13000000];
        double E[13000000];
    
        int i = 0;
    
        // Array initialization
    
    /* Earlier CAPC producer/initializer replayed on host. */
        for (i = 0; i <= 12999999; i += 1) {
            A[i] = ((double)i);
            B[i] = ((double)(i + 1));
        }
    /* Earlier CAPC compute region omitted from standalone host replay. */

    /* === GPU/OpenACC Runtime Initialization === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    acc_init(acc_device_nvidia);
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Device allocation only (no data movement) === */
    #pragma acc enter data create(D[0:13000000], A[0:13000000], B[0:13000000])
    #pragma acc wait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update device(A[0:13000000], B[0:13000000])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 3 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma acc parallel loop auto gang vector num_gangs(50782) vector_length(256) present(D[0:13000000], A[0:13000000], B[0:13000000])
        for (i = 0; i <= 12999999; i += 1) {
            D[i] = A[i] - B[i];
        }
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Required Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update self(D[0:13000000])
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

    #pragma acc exit data delete(D[0:13000000], A[0:13000000], B[0:13000000])
    #pragma acc wait

    /* Runtime shutdown is cleanup and is intentionally not part of isolated time. */
    acc_shutdown(acc_device_nvidia);

    return 0;
}
