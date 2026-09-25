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

#define N 1

int main(void)
{
    struct timespec __capc_t_start, __capc_t_end;
    double __capc_t_init = 0.0;
    double __capc_t_in = 0.0;
    double __capc_t_gpu = 0.0;
    double __capc_t_out = 0.0;

    /* Target Region 2; original function: main() */
    /* === Host-only input/setup replay (NOT timed) === */
    static int assets[N][N];
        static int risk[N][N];
        static int normalized[N][N];
        static int category[N][N];
    
        int i, j;
    
        // Initialize asset data
    /* Earlier CAPC producer/initializer replayed on host. */
        for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                assets[i][j] = (i * 19 + j * 7) % 1000;
            }
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
    #pragma omp target enter data map(alloc:assets[0:N][0:N], risk[0:N][0:N])
    #pragma omp taskwait

    /* === Required Transfer In (Host -> Device) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma omp target update to(assets[0:N][0:N])
    #pragma omp taskwait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_in = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Isolated Kernel Timing for Target Region 2 === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);

    #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for collapse(2) map(present, alloc:assets[0:N][0:N], risk[0:N][0:N])        
    for (i = 0; i < N; i++)
        {
            for (j = 0; j < N; j++)
            {
                risk[i][j] = assets[i][j] + 1;
            }
        }
    #pragma capc profitability_region end

    #pragma omp taskwait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_gpu = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    /* === Required Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_start);
    #pragma omp target update from(risk[0:N][0:N])
    #pragma omp taskwait
    clock_gettime(CLOCK_MONOTONIC, &__capc_t_end);
    __capc_t_out = (__capc_t_end.tv_sec - __capc_t_start.tv_sec) + (__capc_t_end.tv_nsec - __capc_t_start.tv_nsec) / 1e9;

    double __capc_t_total = __capc_t_init + __capc_t_in + __capc_t_gpu + __capc_t_out;
    printf("Region 2 Execution Breakdown:\n");
    printf("  - GPU Initialization : %f seconds\n", __capc_t_init);
    printf("  - Transfer In  (H2D): %f seconds\n", __capc_t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", __capc_t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", __capc_t_out);
    printf("  - Isolated Region Time: %f seconds\n", __capc_t_total);

    #pragma omp target exit data map(delete:assets[0:N][0:N], risk[0:N][0:N])
    #pragma omp taskwait

    /* Device cleanup is intentionally not part of isolated time. */
    return 0;
}
