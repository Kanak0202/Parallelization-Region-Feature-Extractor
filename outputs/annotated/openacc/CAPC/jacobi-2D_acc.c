//2D Jacobi Computation

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
static double __capc_h2d_A = -1.0;
static double __capc_d2h_A = -1.0;
static double __capc_h2d_B = -1.0;
static double __capc_d2h_B = -1.0;
/* ---- end globals ---- */

#define n 5

int main()
{
	int i,j;

	double A[n][n],B[n][n];

	//Array initialization
    #pragma capc profitability_region begin
    #pragma omp parallel for collapse(2) private(i,j)
	for(i=0;i<n;i++)
	{
		for(j=0;j<n;j++)
		{
			A[i][j]=(double)(0.1*i+j);
			B[i][j]=(double)(0.2*j+i);
			printf("");
		}
	}
    #pragma capc profitability_region end

    #pragma acc enter data copyin(A[0:n][0:n],B[0:n][0:n])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = __capc_wtime();
  #pragma acc update device(A[0:n])
  __capc_h2d_A = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'A': %.6f s\n", __capc_h2d_A); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(A[0:n])
  __capc_d2h_A = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'A': %.6f s\n", __capc_d2h_A); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(B[0:n])
  __capc_h2d_B = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'B': %.6f s\n", __capc_h2d_B); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(B[0:n])
  __capc_d2h_B = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'B': %.6f s\n", __capc_d2h_B); }
/* ---- end calibration ---- */


	//Computations
    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(2) present(A[0:n][0:n],B[0:n][0:n])
	for (i = 1; i < n-1; i++)
		for (j = 1; j < n - 1; j++)
			B[i][j] = 0.2 * (A[i][j] + A[i][j-1] + A[i][1+j] + A[1+i][j] + A[i-1][j]);

double __capc_t1 = __capc_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
    #pragma capc profitability_region end

    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(2) present(A[0:n][0:n],B[0:n][0:n])
	for (i = 1; i < n - 1; i++)
		for (j = 1; j < n - 1; j++)
			A[i][j] = 0.2 * (B[i][j] + B[i][j-1] + B[i][1+j] + B[1+i][j] + B[i-1][j]);

double __capc_t1 = __capc_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
    #pragma capc profitability_region end

    #pragma acc update self(A[0:n][0:n],B[0:n][0:n])

	printf("\nMatrix A :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf",A[i][j]);

	printf("\nMatrix B :\n");
	for (i = 0; i < n; i++)
		for(j=0;j<n;j++)
			printf("%lf ",B[i][j]);

	printf("\n");

    #pragma acc exit data delete(A[0:n][0:n],B[0:n][0:n])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  printf("region_0 (pragma at original line 30): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
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
  printf("region_1 (pragma at original line 38): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
}
/* ---- end report ---- */

	return 0;
}