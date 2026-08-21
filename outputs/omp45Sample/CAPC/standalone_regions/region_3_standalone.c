#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

/* ============================================================
 * Original source support code (original main removed)
 * ============================================================ */
#include <stdio.h>

#define N 10000000

double A[N];

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 3; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    int i;
        double sum = 0.0;
    
    
    
        /* =========================================================
           Region 1: Write-only
           ========================================================= */
    /* Earlier CAPC producer/initializer replayed on host. */
        for (i = 0; i < N; i++)
        {
            A[i] = (double)i * 0.5;
        }
    /* Earlier CAPC compute region omitted from standalone host replay. */

    /* === Synthetic initialization for inputs whose prior expensive CAPC producer was omitted === */
    /* Synthetic valid input for 'A': prior CAPC producer was skipped. */
    for (size_t __capc_z0_0 = 0; __capc_z0_0 < (size_t)(N); ++__capc_z0_0) {
        A[(0) + __capc_z0_0] = 0;
    }

    /* === GPU/OpenMP Runtime Initialization === */
    int __capc_device = omp_get_default_device();
    void *__capc_init_ptr = NULL;
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    __capc_init_ptr = omp_target_alloc(1, __capc_device);
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_init = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;
    if (__capc_init_ptr != NULL) omp_target_free(__capc_init_ptr, __capc_device);

    /* === Device allocation only (no data movement) === */
    #pragma omp target enter data map(alloc:A[0:N])
    #pragma omp taskwait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma omp target update to(A[0:N])
    #pragma omp taskwait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 3 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for reduction(+:sum) map(alloc:A[0:N])
        for (i = 0; i < N; i++)
        {
            sum += A[i];
        }
    #pragma capc profitability_region end

    #pragma omp taskwait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* D2H skipped: target does not modify any detected array. */

    double __capc_t_total = __capc_t_init + __capc_t_in + __capc_t_gpu + __capc_t_out;
    printf("Region 3 Execution Breakdown:\n");
    printf("  - GPU Initialization : %f seconds\n", __capc_t_init);
    printf("  - Transfer In  (H2D): %f seconds\n", __capc_t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", __capc_t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", __capc_t_out);
    printf("  - Isolated Region Time: %f seconds\n", __capc_t_total);

    #pragma omp target exit data map(delete:A[0:N])
    #pragma omp taskwait

    /* Device cleanup is intentionally not part of isolated time. */
    return 0;
}
