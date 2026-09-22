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

#define N 1

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 3; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    static int a[N], b[N], normalized[N], category[N];
        int i;
    
        // Initialize arrays
    /* Earlier CAPC producer/initializer replayed on host. */
        for (i = 0; i < N; i++)
        {
            a[i] = (i * 17 + 13) % 1000;
            b[i] = (i * 23 + 7) % 1000;
        }
    /* Earlier CAPC compute region omitted from standalone host replay. */

    /* === Synthetic initialization for inputs whose prior expensive CAPC producer was omitted === */
    /* Synthetic valid input for 'normalized': prior CAPC producer was skipped. */
    for (size_t __capc_z0_0 = 0; __capc_z0_0 < (size_t)(N); ++__capc_z0_0) {
        normalized[(0) + __capc_z0_0] = 0;
    }

    /* === GPU/OpenACC Runtime Initialization === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    acc_init(acc_device_nvidia);
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Device allocation only (no data movement) === */
    #pragma acc enter data create(normalized[0:N], category[0:N])
    #pragma acc wait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update device(normalized[0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 3 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma acc parallel loop present(normalized[0:N], category[0:N])
        for (i = 0; i < N; i++)
        {
            if (normalized[i] > 300)
                category[i] = 3;
            else if (normalized[i] > 150)
                category[i] = 2;
            else if (normalized[i] > 50)
                category[i] = 1;
            else
                category[i] = 0;
        }
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Required Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma acc update self(category[0:N])
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

    #pragma acc exit data delete(normalized[0:N], category[0:N])
    #pragma acc wait

    /* Runtime shutdown is cleanup and is intentionally not part of isolated time. */
    acc_shutdown(acc_device_nvidia);

    return 0;
}
