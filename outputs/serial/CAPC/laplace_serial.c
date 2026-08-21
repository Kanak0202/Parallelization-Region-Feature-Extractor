#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>

// grid size
#define GRIDY    4096
#define GRIDX    4096

// number of simulation iterations
#ifndef NUM_ITERATIONS
#define NUM_ITERATIONS 100
#endif

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// smallest permitted change in temperature
#define MAX_TEMP_ERROR 0.02

double T_new[GRIDX+2][GRIDY+2]; // temperature grid
double T[GRIDX+2][GRIDY+2];     // temperature grid from last iteration

// initialization routine
void init();

int main() {

    int i, j;                                            // grid indexes
    int max_iterations = NUM_ITERATIONS;                 // maximal number of iterations
    int iteration = 15;                                  // iteration
    double dt = 100;                                     // largest change in temperature
    struct timeval start_time, stop_time, elapsed_time;  // timers

    gettimeofday(&start_time, NULL);

    init();

    // simulation iterations
    while (dt > MAX_TEMP_ERROR && iteration <= max_iterations) {

        // main computational kernel, average over neighbours in the grid
        #pragma capc profitability_region begin
        for(i = 1; i <= GRIDX; i++)
            for(j = 1; j <= GRIDY; j++)
                T_new[i][j] = 0.25 * (T[i+1][j] + T[i-1][j] +
                                      T[i][j+1] + T[i][j-1]);
        #pragma capc profitability_region end

        // reset dt
        dt = 0.0;

        // compute the largest change and copy T_new to T
        #pragma capc profitability_region begin
        for(i = 1; i <= GRIDX; i++) {
            for(j = 1; j <= GRIDY; j++) {
                dt = MAX(fabs(T_new[i][j] - T[i][j]), dt);
                T[i][j] = T_new[i][j];
            }
        }
        #pragma capc profitability_region end

        // periodically print largest change
        if((iteration % 100) == 0)
            printf("Iteration %4.0d, dt %f\n", iteration, dt);

        iteration++;
    }

    gettimeofday(&stop_time, NULL);
    timersub(&stop_time, &start_time, &elapsed_time);

    printf("Total time was %f seconds.\n",
           elapsed_time.tv_sec + elapsed_time.tv_usec / 1000000.0);

    return 0;
}


// initialize grid and boundary conditions
void init() {

    int i, j;

    #pragma capc profitability_region begin
    for(i = 0; i <= GRIDX+1; i++) {
        for(j = 0; j <= GRIDY+1; j++) {
            T[i][j] = 0.0;
        }
    }
    #pragma capc profitability_region end

    // these boundary conditions never change throughout run

    // set left side to 0 and right to a linear increase
    #pragma capc profitability_region begin
    for(i = 0; i <= GRIDX+1; i++) {
        T[i][0] = 0.0;
        T[i][GRIDY+1] = (128.0/GRIDX) * i;
    }
    #pragma capc profitability_region end

    // set top to 0 and bottom to linear increase
    #pragma capc profitability_region begin
    for(j = 0; j <= GRIDY+1; j++) {
        T[0][j] = 0.0;
        T[GRIDX+1][j] = (128.0/GRIDY) * j;
    }
    #pragma capc profitability_region end
}