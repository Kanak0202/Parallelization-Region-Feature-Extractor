#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

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
static double __capc_region_4_total = 0.0;
static long   __capc_region_4_count = 0;
static double __capc_h2d_T = -1.0;
static double __capc_d2h_T = -1.0;
static double __capc_h2d_T_new = -1.0;
static double __capc_d2h_T_new = -1.0;
/* ---- end globals ---- */


// grid size
#define GRIDY    4096
#define GRIDX    100000

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// smallest permitted change in temperature
#define MAX_TEMP_ERROR 0.02

double T_new[GRIDX+2][GRIDY+2]; // temperature grid
double T[GRIDX+2][GRIDY+2];     // temperature grid from last iteration

//   initialisation routine
void init();

int main(int argc, char *argv[]) {

    int i, j;                                            // grid indexes
    int max_iterations;                                  // maximal number of iterations
    int iteration=15;                                     // iteration
    double dt=100;                                       // largest change in temperature
    struct timeval start_time, stop_time, elapsed_time;  // timers

    if(argc!=2) {
      printf("Usage: %s number_of_iterations\n",argv[0]);
      exit(1);
    } else {
      max_iterations=atoi(argv[1]);
    }

    gettimeofday(&start_time,NULL); 

    #pragma acc enter data create(T[0:GRIDX+2][0:GRIDY+2],T_new[0:GRIDX+2][0:GRIDY+2])

/* ---- capc timing instrumentation: one-shot transfer calibration ---- */
{ double __t0 = __capc_wtime();
  #pragma acc update device(T[0:GRIDX+2])
  __capc_h2d_T = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'T': %.6f s\n", __capc_h2d_T); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(T[0:GRIDX+2])
  __capc_d2h_T = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'T': %.6f s\n", __capc_d2h_T); }
{ double __t0 = __capc_wtime();
  #pragma acc update device(T_new[0:GRIDX+2])
  __capc_h2d_T_new = __capc_wtime() - __t0;
  printf("[capc] H2D transfer time for 'T_new': %.6f s\n", __capc_h2d_T_new); }
{ double __t0 = __capc_wtime();
  #pragma acc update self(T_new[0:GRIDX+2])
  __capc_d2h_T_new = __capc_wtime() - __t0;
  printf("[capc] D2H transfer time for 'T_new': %.6f s\n", __capc_d2h_T_new); }
/* ---- end calibration ---- */


    init();                  

    // simulation iterations
    while ( dt > MAX_TEMP_ERROR && iteration <= max_iterations ) {

        // main computational kernel, average over neighbours in the grid
        #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
        #pragma acc parallel loop collapse(2) present(T[0:GRIDX+2][0:GRIDY+2],T_new[0:GRIDX+2][0:GRIDY+2])
        for(i = 1; i <= GRIDX; i++) 
            for(j = 1; j <= GRIDY; j++) 
                T_new[i][j] = 0.25 * (T[i+1][j] + T[i-1][j] +
                                            T[i][j+1] + T[i][j-1]);
double __capc_t1 = __capc_wtime(); __capc_region_0_total += (__capc_t1 - __capc_t0); __capc_region_0_count += 1; }
        #pragma capc profitability_region end

        // reset dt
        dt = 0.0;

        // compute the largest change and copy T_new to T
        #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
        #pragma acc parallel loop collapse(2) reduction(max:dt) present(T[0:GRIDX+2][0:GRIDY+2],T_new[0:GRIDX+2][0:GRIDY+2])
        for(i = 1; i <= GRIDX; i++){
            for(j = 1; j <= GRIDY; j++){
	      dt = MAX( fabs(T_new[i][j]-T[i][j]), dt);
	      T[i][j] = T_new[i][j];
            }
        }
double __capc_t1 = __capc_wtime(); __capc_region_1_total += (__capc_t1 - __capc_t0); __capc_region_1_count += 1; }
        #pragma capc profitability_region end

        // periodically print largest change
        if((iteration % 100) == 0) 
            printf("Iteration %4.0d, dt %f\n",iteration,dt);
        
	iteration++;
    }

    #pragma acc exit data delete(T[0:GRIDX+2][0:GRIDY+2],T_new[0:GRIDX+2][0:GRIDY+2])

    gettimeofday(&stop_time,NULL);
    timersub(&stop_time, &start_time, &elapsed_time); // measure time

    printf("Total time was %f seconds.\n", elapsed_time.tv_sec+elapsed_time.tv_usec/1000000.0);


/* ---- capc timing instrumentation: report ---- */
{
  double __resident = (__capc_region_0_count > 0) ? (__capc_region_0_total / __capc_region_0_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_T > 0) __xfer += __capc_h2d_T;
  if (__capc_d2h_T > 0) __xfer += __capc_d2h_T;
  if (__capc_h2d_T_new > 0) __xfer += __capc_h2d_T_new;
  if (__capc_d2h_T_new > 0) __xfer += __capc_d2h_T_new;
  printf("region_0 (pragma at original line 47): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_0_count);
  printf("    T: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T > 0 ? __capc_h2d_T : 0.0, __capc_d2h_T > 0 ? __capc_d2h_T : 0.0);
  printf("    T_new: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T_new > 0 ? __capc_h2d_T_new : 0.0, __capc_d2h_T_new > 0 ? __capc_d2h_T_new : 0.0);
}
{
  double __resident = (__capc_region_1_count > 0) ? (__capc_region_1_total / __capc_region_1_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_T > 0) __xfer += __capc_h2d_T;
  if (__capc_d2h_T > 0) __xfer += __capc_d2h_T;
  if (__capc_h2d_T_new > 0) __xfer += __capc_h2d_T_new;
  if (__capc_d2h_T_new > 0) __xfer += __capc_d2h_T_new;
  printf("region_1 (pragma at original line 59): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_1_count);
  printf("    T: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T > 0 ? __capc_h2d_T : 0.0, __capc_d2h_T > 0 ? __capc_d2h_T : 0.0);
  printf("    T_new: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T_new > 0 ? __capc_h2d_T_new : 0.0, __capc_d2h_T_new > 0 ? __capc_d2h_T_new : 0.0);
}
{
  double __resident = (__capc_region_2_count > 0) ? (__capc_region_2_total / __capc_region_2_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_T > 0) __xfer += __capc_h2d_T;
  if (__capc_d2h_T > 0) __xfer += __capc_d2h_T;
  printf("region_2 (pragma at original line 91): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_2_count);
  printf("    T: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T > 0 ? __capc_h2d_T : 0.0, __capc_d2h_T > 0 ? __capc_d2h_T : 0.0);
}
{
  double __resident = (__capc_region_3_count > 0) ? (__capc_region_3_total / __capc_region_3_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_T > 0) __xfer += __capc_h2d_T;
  if (__capc_d2h_T > 0) __xfer += __capc_d2h_T;
  printf("region_3 (pragma at original line 103): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_3_count);
  printf("    T: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T > 0 ? __capc_h2d_T : 0.0, __capc_d2h_T > 0 ? __capc_d2h_T : 0.0);
}
{
  double __resident = (__capc_region_4_count > 0) ? (__capc_region_4_total / __capc_region_4_count) : 0.0;
  double __xfer = 0.0;
  if (__capc_h2d_T > 0) __xfer += __capc_h2d_T;
  if (__capc_d2h_T > 0) __xfer += __capc_d2h_T;
  printf("region_4 (pragma at original line 112): resident_avg=%.6f s isolated_avg=%.6f s calls=%ld\n", __resident, __resident + __xfer, __capc_region_4_count);
  printf("    T: h2d=%.6f s d2h=%.6f s\n", __capc_h2d_T > 0 ? __capc_h2d_T : 0.0, __capc_d2h_T > 0 ? __capc_d2h_T : 0.0);
}
/* ---- end report ---- */

    return 0;
}


// initialize grid and boundary conditions
void init(){

    int i,j;
    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop collapse(2) present(T[0:GRIDX+2][0:GRIDY+2])
    for(i = 0; i <= GRIDX+1; i++){
        for (j = 0; j <= GRIDY+1; j++){
            T[i][j] = 0.0;
        }
    }
double __capc_t1 = __capc_wtime(); __capc_region_2_total += (__capc_t1 - __capc_t0); __capc_region_2_count += 1; }
    #pragma capc profitability_region end

    // these boundary conditions never change throughout run

    // set left side to 0 and right to a linear increase
    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop present(T[0:GRIDX+2][0:GRIDY+2])
    for(i = 0; i <= GRIDX+1; i++) {
        T[i][0] = 0.0;
        T[i][GRIDY+1] = (128.0/GRIDX)*i;
    }
double __capc_t1 = __capc_wtime(); __capc_region_3_total += (__capc_t1 - __capc_t0); __capc_region_3_count += 1; }
    #pragma capc profitability_region end
    
    // set top to 0 and bottom to linear increase
    #pragma capc profitability_region begin
{ double __capc_t0 = __capc_wtime();
    #pragma acc parallel loop present(T[0:GRIDX+2][0:GRIDY+2])
    for(j = 0; j <= GRIDY+1; j++) {
        T[0][j] = 0.0;
        T[GRIDX+1][j] = (128.0/GRIDY)*j;
    }
double __capc_t1 = __capc_wtime(); __capc_region_4_total += (__capc_t1 - __capc_t0); __capc_region_4_count += 1; }
    #pragma capc profitability_region end
}