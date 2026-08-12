//Vector Arithmatic

#include <stdio.h>

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
static double __capc_h2d_A = -1.0;
static double __capc_d2h_A = -1.0;
static double __capc_h2d_B = -1.0;
static double __capc_d2h_B = -1.0;
static double __capc_h2d_C = -1.0;
static double __capc_d2h_C = -1.0;
static double __capc_h2d_D = -1.0;
static double __capc_d2h_D = -1.0;
static double __capc_h2d_E = -1.0;
static double __capc_d2h_E = -1.0;
/* ---- end globals ---- */

#define SIZE 5

int main()
{
	double A[SIZE],B[SIZE],C[SIZE],D[SIZE],E[SIZE];
	int i = 0;

#pragma acc enter data create(A[0:SIZE],B[0:SIZE],C[0:SIZE],D[0:SIZE],E[0:SIZE])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = __capc_wtime();
  #pragma acc update device(A[0:SIZE])
  __capc_h2d_A = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'A': %.6f s\n", __capc_h2d_A); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(A[0:SIZE])
  __capc_d2h_A = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'A': %.6f s\n", __capc_d2h_A); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(B[0:SIZE])
  __capc_h2d_B = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'B': %.6f s\n", __capc_h2d_B); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(B[0:SIZE])
  __capc_d2h_B = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'B': %.6f s\n", __capc_d2h_B); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(C[0:SIZE])
  __capc_h2d_C = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'C': %.6f s\n", __capc_h2d_C); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(C[0:SIZE])
  __capc_d2h_C = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'C': %.6f s\n", __capc_d2h_C); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(D[0:SIZE])
  __capc_h2d_D = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'D': %.6f s\n", __capc_h2d_D); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(D[0:SIZE])
  __capc_d2h_D = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'D': %.6f s\n", __capc_d2h_D); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(E[0:SIZE])
  __capc_h2d_E = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'E': %.6f s\n", __capc_h2d_E); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(E[0:SIZE])
  __capc_d2h_E = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'E': %.6f s\n", __capc_d2h_E); }
/* ---- end calibration ---- */


	//Array initialization
#pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
#pragma acc parallel loop present(A[0:SIZE],B[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		A[i] = (double)i;
		B[i] = (double)(i+1);
	}
double __capc_t1 = __capc_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
#pragma capc profitability_region end

	//C=A+B
#pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
#pragma acc parallel loop present(A[0:SIZE],B[0:SIZE],C[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		C[i]=A[i]+B[i];
	}
double __capc_t1 = __capc_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
#pragma capc profitability_region end

#pragma acc update self(A[0:SIZE],B[0:SIZE],C[0:SIZE])

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (C[i] !=A[i] + B[i])
		{
			printf("Add : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Add : Everything seems to work fine! \n");
	}

	//D=A-B	
#pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
#pragma acc parallel loop present(A[0:SIZE],B[0:SIZE],D[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		D[i]=A[i]-B[i];
	}
double __capc_t1 = __capc_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
#pragma capc profitability_region end

#pragma acc update self(D[0:SIZE])

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (D[i] !=A[i] - B[i])
		{
			printf("Sub : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Sub : Everything seems to work fine! \n");
	}

	//E=A*B
#pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
#pragma acc parallel loop present(A[0:SIZE],B[0:SIZE],E[0:SIZE])
	for (i=0; i<SIZE; ++i)
	{
		E[i]=A[i]*B[i];
	}
double __capc_t1 = __capc_wtime(); __capc_region_3_total += (__capc_t1 - __capc_t0); __capc_region_3_count += 1; }
#pragma capc profitability_region end

#pragma acc update self(E[0:SIZE])

	//Verify result
	for (i=0; i<SIZE; ++i)
	{
		if (E[i] !=A[i] * B[i])
		{
			printf("Mult : Something didn't work correctly!\n");
			break;
		}
	}

	if (i == SIZE)
	{
		printf("Mult : Everything seems to work fine! \n");
	}

#pragma acc exit data delete(A[0:SIZE],B[0:SIZE],C[0:SIZE],D[0:SIZE],E[0:SIZE])


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  printf("region_0 (pragma at original line 15): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
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
  if (__capc_h2d_C > 0) __xfer += __capc_h2d_C;
  if (__capc_d2h_C > 0) __xfer += __capc_d2h_C;
  printf("region_1 (pragma at original line 25): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
  printf("    C: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_C > 0 ? __capc_h2d_C : 0.0, __capc_d2h_C > 0 ? __capc_d2h_C : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  if (__capc_h2d_D > 0) __xfer += __capc_h2d_D;
  if (__capc_d2h_D > 0) __xfer += __capc_d2h_D;
  printf("region_2 (pragma at original line 51): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
  printf("    D: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_D > 0 ? __capc_h2d_D : 0.0, __capc_d2h_D > 0 ? __capc_d2h_D : 0.0);
}
{
  double __resident = (__capc_region_3_count > 0) ? (__capc_region_3_total / __capc_region_3_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_A > 0) __xfer += __capc_h2d_A;
  if (__capc_d2h_A > 0) __xfer += __capc_d2h_A;
  if (__capc_h2d_B > 0) __xfer += __capc_h2d_B;
  if (__capc_d2h_B > 0) __xfer += __capc_d2h_B;
  if (__capc_h2d_E > 0) __xfer += __capc_h2d_E;
  if (__capc_d2h_E > 0) __xfer += __capc_d2h_E;
  printf("region_3 (pragma at original line 77): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_3_count);
  printf("    A: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_A > 0 ? __capc_h2d_A : 0.0, __capc_d2h_A > 0 ? __capc_d2h_A : 0.0);
  printf("    B: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_B > 0 ? __capc_h2d_B : 0.0, __capc_d2h_B > 0 ? __capc_d2h_B : 0.0);
  printf("    E: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_E > 0 ? __capc_h2d_E : 0.0, __capc_d2h_E > 0 ? __capc_d2h_E : 0.0);
}
/* ---- end report ---- */

	return 0;
}