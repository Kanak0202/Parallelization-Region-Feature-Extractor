#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <omp.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#define N 49000000
#define T 500

double a[N];
double b[N];

void init_array()
{
        int i, j;
        #pragma capc profitability_region begin
        #pragma omp target teams distribute parallel for map(alloc:a[0:N],b[0:N]) private(i)
        for (i=0; i<N; i++)
        {
                a[i] = ((double)i)/N;
                b[i] = ((double)i+1)/N;
        }
        #pragma capc profitability_region end
}

void print_array()
{
        int i, j;

        for (i=0; i<N; i++)
                printf("%lf ", a[i]);

        printf("\n");

        for (i=0; i<N; i++)
                printf("%lf ", b[i]);
}


int main()
{

    int i, j, k, t;
    struct timespec start_h2d, end_h2d, start_exec, end_exec, start_d2h, end_d2h;
    double t_h2d = 0.0, t_exec = 0.0, t_d2h = 0.0;

    /* === STAGE 1 & 2: Interleaved Setup & Prerequisite Regions === */
    #pragma omp target enter data map(alloc:a[0:N],b[0:N])

        init_array();

    /* === STAGE 3a: Pre-timing H2D Copyin skipped (write-only or has prior dependencies) === */

    /* === STAGE 3b: Isolated Execution for Target Region 2 === */
    clock_gettime(CLOCK_MONOTONIC, &start_exec);

        #pragma capc profitability_region begin
    #pragma omp target teams distribute parallel for map(alloc:a[0:N],b[0:N]) private(i)
                for (i = 2; i < N - 1; i++)
                {
                        b[i] = 0.33333 * (a[i-1] + a[i] + a[i + 1]);
                }
    #pragma capc profitability_region end

    clock_gettime(CLOCK_MONOTONIC, &end_exec);
    t_exec = (end_exec.tv_sec - start_exec.tv_sec) + (end_exec.tv_nsec - start_exec.tv_nsec) / 1e9;

    /* === STAGE 3c: Timed Device -> Host (D2H) Data Transfer === */
    clock_gettime(CLOCK_MONOTONIC, &start_d2h);
    #pragma omp target update from(a[0:N], b[0:N])
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