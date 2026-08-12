#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

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
/* ---- end globals ---- */

#define n 10

int main()
{
	int i, j, k;

	float A[n][n][n],B[n][n][n];

    #pragma acc enter data create(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
/* ---- end calibration ---- */


    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(3) present(A,B)
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				A[i][j][k] = B[i][j][k] = (float) (i + j + (n-k))* 10 / (n);
double __capc_t1 = __capc_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(3) present(A,B)
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
double __capc_t1 = __capc_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(3) present(A,B)
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
double __capc_t1 = __capc_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:n][0:n][0:n],B[0:n][0:n][0:n])

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

    #pragma acc exit data delete(A[0:n][0:n][0:n],B[0:n][0:n][0:n])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  printf("region_0 (pragma at original line 17): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  printf("region_1 (pragma at original line 25): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  printf("region_2 (pragma at original line 39): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
}
/* ---- end report ---- */

	return 0;
}