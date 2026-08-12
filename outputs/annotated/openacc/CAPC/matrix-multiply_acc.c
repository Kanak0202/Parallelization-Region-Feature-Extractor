#include<stdio.h>

/* ---- capc timing instrumentation: globals ---- */
#include <sys/time.h>
static double __capc_wtime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (double)tv.tv_sec + (double)tv.tv_usec * 1e-6;
}
static double __capc_region_0_total = 0.0;
static long   __capc_region_0_count = 0;
static double __capc_region_1_total = 0.0;
static long   __capc_region_1_count = 0;
static double __capc_region_2_total = 0.0;
static long   __capc_region_2_count = 0;
static double __capc_region_3_total = 0.0;
static long   __capc_region_3_count = 0;
static double __capc_h2d_a = -1.0;
static double __capc_d2h_a = -1.0;
static double __capc_h2d_b = -1.0;
static double __capc_d2h_b = -1.0;
static double __capc_h2d_c = -1.0;
static double __capc_d2h_c = -1.0;
/* ---- end globals ---- */

#define N 50
//#define M 10
//#define K 10

int main()
{
	int i,j,k;
	//double a[N][M],b[M][K],c[N][K];
	double a[N][N],b[N][N],c[N][N];

	#pragma acc enter data create(a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = __capc_wtime();
  #pragma acc update device(a[0:N])
  __capc_h2d_a = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'a': %.6f s\n", __capc_h2d_a); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(a[0:N])
  __capc_d2h_a = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'a': %.6f s\n", __capc_d2h_a); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(b[0:N])
  __capc_h2d_b = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'b': %.6f s\n", __capc_h2d_b); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(b[0:N])
  __capc_d2h_b = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'b': %.6f s\n", __capc_d2h_b); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(c[0:N])
  __capc_h2d_c = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'c': %.6f s\n", __capc_h2d_c); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(c[0:N])
  __capc_d2h_c = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'c': %.6f s\n", __capc_d2h_c); }
/* ---- end calibration ---- */


	#pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
	#pragma acc parallel loop collapse(2) present(a[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			a[i][j]=i+1;
		}
	}
double __capc_t1 = __capc_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
	#pragma acc parallel loop collapse(2) present(b[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			b[i][j]=j+1;
		}
	}
double __capc_t1 = __capc_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();

	#pragma acc parallel loop collapse(2) present(c[0:N][0:N])
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			c[i][j]=0;
		}
	}
double __capc_t1 = __capc_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
	#pragma acc parallel loop collapse(2) present(a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])
	for (i = 0; i < N; i++)
		for (j = 0; j < N; j++)
			for (k = 0; k < N; k++)
				c[i][j]= c[i][j]+a[i][k]*b[k][j];
double __capc_t1 = __capc_wtime(); __capc_region_3_total += (__capc_t1 - __capc_t0); __capc_region_3_count += 1; }
    #pragma capc profitability_region end

	#pragma acc update self(c[0:N][0:N])

    
	for(i=0;i<N;i++)
	{
		for(j=0;j<N;j++)
		{
			printf("%lf\t",c[i][j]);
		}
		printf("\n");
	}

	#pragma acc exit data delete(a[0:N][0:N],b[0:N][0:N],c[0:N][0:N])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  printf("region_0 (pragma at original line 15): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_b > 0) __xfer += __capc_h2d_b;
  if (__capc_d2h_b > 0) __xfer += __capc_d2h_b;
  printf("region_1 (pragma at original line 26): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    b: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_b > 0 ? __capc_h2d_b : 0.0, __capc_d2h_b > 0 ? __capc_d2h_b : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_c > 0) __xfer += __capc_h2d_c;
  if (__capc_d2h_c > 0) __xfer += __capc_d2h_c;
  printf("region_2 (pragma at original line 38): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    c: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_c > 0 ? __capc_h2d_c : 0.0, __capc_d2h_c > 0 ? __capc_d2h_c : 0.0);
}
{
  double __resident = (__capc_region_3_count > 0) ? (__capc_region_3_total / __capc_region_3_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_a > 0) __xfer += __capc_h2d_a;
  if (__capc_d2h_a > 0) __xfer += __capc_d2h_a;
  if (__capc_h2d_b > 0) __xfer += __capc_h2d_b;
  if (__capc_d2h_b > 0) __xfer += __capc_d2h_b;
  if (__capc_h2d_c > 0) __xfer += __capc_h2d_c;
  if (__capc_d2h_c > 0) __xfer += __capc_d2h_c;
  printf("region_3 (pragma at original line 49): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_3_count);
  printf("    a: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_a > 0 ? __capc_h2d_a : 0.0, __capc_d2h_a > 0 ? __capc_d2h_a : 0.0);
  printf("    b: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_b > 0 ? __capc_h2d_b : 0.0, __capc_d2h_b > 0 ? __capc_d2h_b : 0.0);
  printf("    c: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_c > 0 ? __capc_h2d_c : 0.0, __capc_d2h_c > 0 ? __capc_d2h_c : 0.0);
}
/* ---- end report ---- */

	return 0;
}