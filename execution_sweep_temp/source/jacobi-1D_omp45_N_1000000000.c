#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

/* ---- capc timing instrumentation: globals ---- */
#include <omp.h>
static double __capc_region_0_total = 0.0;
static long   __capc_region_0_count = 0;
static double __capc_region_1_total = 0.0;
static long   __capc_region_1_count = 0;
static double __capc_region_2_total = 0.0;
static long   __capc_region_2_count = 0;
static double __capc_h2d_a = -1.0;
static double __capc_d2h_a = -1.0;
static double __capc_h2d_b = -1.0;
static double __capc_d2h_b = -1.0;
/* ---- end globals ---- */


#define N 1000000000
#define T 500

double a[N];
double b[N];

void init_array()
{
        int i, j;
        #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
        #pragma omp target teams distribute parallel for map(alloc:a[0:N],b[0:N]) private(i)
        for (i=0; i<N; i++)
        {
                a[i] = ((double)i)/N;
                b[i] = ((double)i+1)/N;
        }
double __capc_t1 = omp_get_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
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
        int t, i, j;

        #pragma omp target enter data map(alloc:a[0:N],b[0:N])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = omp_get_wtime();
  #pragma omp target update to(a[0:N])
  __capc_h2d_a = omp_get_wtime() - __t0;
  printf("[capc] H2D transfer time for 'a': %.6f s\n", __capc_h2d_a); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update from(a[0:N])
  __capc_d2h_a = omp_get_wtime() - __t0;
  printf("[capc] D2H transfer time for 'a': %.6f s\n", __capc_d2h_a); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update to(b[0:N])
  __capc_h2d_b = omp_get_wtime() - __t0;
  printf("[capc] H2D transfer time for 'b': %.6f s\n", __capc_h2d_b); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update from(b[0:N])
  __capc_d2h_b = omp_get_wtime() - __t0;
  printf("[capc] D2H transfer time for 'b': %.6f s\n", __capc_d2h_b); }
/* ---- end calibration ---- */


        init_array();

        for (t = 0; t < T; t++)
        {
                
                #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
                #pragma omp target teams distribute parallel for map(alloc:a[0:N],b[0:N]) private(i)
                for (i = 2; i < N - 1; i++)
                {
                        b[i] = 0.33333 * (a[i-1] + a[i] + a[i + 1]);
                }
double __capc_t1 = omp_get_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
                #pragma capc profitability_region end
                
                
                #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
                #pragma omp target teams distribute parallel for map(alloc:a[0:N],b[0:N]) private(i)
                for (i = 2; i < N - 1; i++)
                {
                        a[i] = b[i];
                }
double __capc_t1 = omp_get_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
                #pragma capc profitability_region end
                
        }

        #pragma omp target update from(a[0:N],b[0:N])

//      print_array();

       printf("a[0]=%lf\n",a[0]);
        printf("a[%d]=%lf\n",N-2,a[N-2]);

        printf("b[0]=%lf\n",b[0]);
        printf("b[%d]=%lf\n",N-1,b[N-1]);

        #pragma omp target exit data map(delete:a[0:N],b[0:N])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_b > 0) __xfer += __capc_h2d_b;
  if (__capc_d2h_b > 0) __xfer += __capc_d2h_b;
  printf("region_0 (pragma at original line 17): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    b: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_b > 0 ? __capc_h2d_b : 0.0, __capc_d2h_b > 0 ? __capc_d2h_b : 0.0);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_b > 0) __xfer += __capc_h2d_b;
  if (__capc_d2h_b > 0) __xfer += __capc_d2h_b;
  printf("region_1 (pragma at original line 51): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    b: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_b > 0 ? __capc_h2d_b : 0.0, __capc_d2h_b > 0 ? __capc_d2h_b : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_b > 0) __xfer += __capc_h2d_b;
  if (__capc_d2h_b > 0) __xfer += __capc_d2h_b;
  printf("region_2 (pragma at original line 60): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    b: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_b > 0 ? __capc_h2d_b : 0.0, __capc_d2h_b > 0 ? __capc_d2h_b : 0.0);
}
/* ---- end report ---- */

        return 0;
}