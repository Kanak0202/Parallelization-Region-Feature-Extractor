#define _GNU_SOURCE
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <stdio.h>

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
        #pragma acc parallel loop present(a[0:N],b[0:N])
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
    struct timespec t_start, t_end;
    double t_in = 0.0, t_gpu = 0.0, t_out = 0.0;

    /* === STAGE 1 & 2: Interleaved Setup & Prerequisite Regions === */
    #pragma acc enter data create(a[0:N],b[0:N])

        init_array();

    // Dependent Region 2
        #pragma capc profitability_region begin
    #pragma acc parallel loop present(a[0:N],b[0:N])
                for (i = 2; i < N - 1; i++)
                {
                        b[i] = 0.33333 * (a[i-1] + a[i] + a[i + 1]);
                }
    #pragma capc profitability_region end
    #pragma acc wait

    /* === Pre-timing Copyin skipped: Region is write-only / copyout or has prior dependencies === */

    /* === Isolated Kernel Timing for Target Region 3 === */
    clock_gettime(CLOCK_MONOTONIC, &t_start);

        #pragma capc profitability_region begin
    #pragma acc parallel loop present(a[0:N],b[0:N])
                for (i = 2; i < N - 1; i++)
                {
                        a[i] = b[i];
                }
    #pragma capc profitability_region end

    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    t_gpu = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    /* === Transfer Out (Device -> Host) === */
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    #pragma acc update self(a[0:N], b[0:N])
    #pragma acc wait
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    t_out = (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    /* === STAGE 4: Reporting Breakdown === */
    double t_total = t_in + t_gpu + t_out;
    printf("Region 3 Execution Breakdown:\n");
    printf("  - Transfer In  (H2D): %f seconds\n", t_in);
    printf("  - Kernel Time (GPU): %f seconds\n", t_gpu);
    printf("  - Transfer Out (D2H): %f seconds\n", t_out);
    printf("  - Total Region Time : %f seconds\n", t_total);

    return 0;
}