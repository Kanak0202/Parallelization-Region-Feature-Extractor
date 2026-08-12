#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ---- capc timing instrumentation: globals ---- */
#include <omp.h>
static double __capc_region_0_total = 0.0;
static long   __capc_region_0_count = 0;
static double __capc_region_1_total = 0.0;
static long   __capc_region_1_count = 0;
static double __capc_region_2_total = 0.0;
static long   __capc_region_2_count = 0;
static double __capc_h2d_A = -1.0;
static double __capc_d2h_A = -1.0;
static double __capc_h2d_B = -1.0;
static double __capc_d2h_B = -1.0;
/* ---- end globals ---- */

#define n 10

int main()
{
	int i, j, k;

	float A[n][n][n],B[n][n][n];

    #pragma omp target enter data map(alloc:A[0:n][0:n][0:n],B[0:n][0:n][0:n])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = omp_get_wtime();
  #pragma omp target update to(A[0:n])
  __capc_h2d_A = omp_get_wtime() - __t0;
  printf("[capc] H2D transfer time for 'A': %.6f s\n", __capc_h2d_A); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update from(A[0:n])
  __capc_d2h_A = omp_get_wtime() - __t0;
  printf("[capc] D2H transfer time for 'A': %.6f s\n", __capc_d2h_A); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update to(B[0:n])
  __capc_h2d_B = omp_get_wtime() - __t0;
  printf("[capc] H2D transfer time for 'B': %.6f s\n", __capc_h2d_B); }
{ double __t0 = omp_get_wtime();
  #pragma omp target update from(B[0:n])
  __capc_d2h_B = omp_get_wtime() - __t0;
  printf("[capc] D2H transfer time for 'B': %.6f s\n", __capc_d2h_B); }
/* ---- end calibration ---- */


    #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(3) private(i,j,k) map(alloc:A[0:n][0:n][0:n],B[0:n][0:n][0:n])
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				A[i][j][k] = B[i][j][k] = (float) (i + j + (n-k))* 10 / (n);
double __capc_t1 = omp_get_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(3) private(i,j,k) map(alloc:A[0:n][0:n][0:n],B[0:n][0:n][0:n])
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				B[i][j][k] = 0.125 * (A[i+1][j][k] - (2.0) * A[i][j][k] + A[i-1][j][k])
					+ 0.125 * (A[i][j+1][k] - (2.0) * A[i][j][k] + A[i][j-1][k])
					+ 0.125 * (A[i][j][k+1] -(2.0) * A[i][j][k] + A[i][j][k-1])
					+ A[i][j][k];
			}
		}
	}
double __capc_t1 = omp_get_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = omp_get_wtime();
    #pragma omp target teams distribute parallel for collapse(3) private(i,j,k) map(alloc:A[0:n][0:n][0:n],B[0:n][0:n][0:n])
	for (i = 1; i < n-1; i++) {
		for (j = 1; j < n-1; j++) {
			for (k = 1; k < n-1; k++) {
				A[i][j][k] = 0.125 * (B[i+1][j][k] - (2.0) * B[i][j][k] + B[i-1][j][k])
					+ 0.125 * (B[i][j+1][k] - (2.0) * B[i][j][k] + B[i][j-1][k])
					+ 0.125 * (B[i][j][k+1] - (2.0) * B[i][j][k] + B[i][j][k-1])
					+ B[i][j][k];
			}
		}
	}
double __capc_t1 = omp_get_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
    #pragma capc profitability_region end

    #pragma omp target update from(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f",A[i][j][k]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			for(k=0;k<n;k++)
				printf("%f ",B[i][j][k]);

	printf("\n");

    #pragma omp target exit data map(delete:A[0:n][0:n][0:n],B[0:n][0:n][0:n])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  printf("region_0 (pragma at original line 17): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  printf("region_1 (pragma at original line 25): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  printf("region_2 (pragma at original line 39): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
}
/* ---- end report ---- */

	return 0;
}