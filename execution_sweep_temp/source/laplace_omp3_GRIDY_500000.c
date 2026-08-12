#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <omp.h>

/* CAPC timing support: generated */
static double __capc_rt[5]={0}; static unsigned long long __capc_rc[5]={0};
static double __capc_tt[1]={0}; static unsigned long long __capc_tc[1]={0};
static const char *__capc_tk[1]={""};
static void __capc_report(void){
 int q; double h=0,d=0; unsigned long long hc=0,dc=0;
 printf("\n===== CAPC TIMING REPORT (omp3) =====\n");
 for(q=0;q<5;q++) if(__capc_rc[q]) printf("Region %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",q,__capc_rt[q],__capc_rc[q],__capc_rt[q]/(double)__capc_rc[q]);
 for(q=0;q<0;q++) if(__capc_tc[q]){
   printf("%s transfer %d: total=%0.9f s, executions=%llu, average=%0.9f s\n",__capc_tk[q],q,__capc_tt[q],__capc_tc[q],__capc_tt[q]/(double)__capc_tc[q]);
   if(__capc_tk[q][0]=='H'){h+=__capc_tt[q];hc+=__capc_tc[q];} else {d+=__capc_tt[q];dc+=__capc_tc[q];}
 }
 if(hc) printf("H2D summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",h,hc,h/(double)hc);
 if(dc) printf("D2H summary: total=%0.9f s, transfers=%llu, average=%0.9f s\n",d,dc,d/(double)dc);
 printf("=======================================\n");
}
/* end CAPC timing support */


// grid size
#define GRIDY    500000
#define GRIDX    4096

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

// smallest permitted change in temperature
#define MAX_TEMP_ERROR 0.02

double T_new[GRIDX+2][GRIDY+2]; // temperature grid
double T[GRIDX+2][GRIDY+2];     // temperature grid from last iteration

//   initialisation routine
void init();

int main(int argc, char *argv[]) {
    atexit(__capc_report);

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

    init();                  

    // simulation iterations
    while ( dt > MAX_TEMP_ERROR && iteration <= max_iterations ) {

        // main computational kernel, average over neighbours in the grid
        #pragma capc profitability_region begin
        double __capc_rs_0=omp_get_wtime();
        #pragma omp parallel for collapse(2) private(i,j)
        for(i = 1; i <= GRIDX; i++) 
            for(j = 1; j <= GRIDY; j++) 
                T_new[i][j] = 0.25 * (T[i+1][j] + T[i-1][j] +
                                            T[i][j+1] + T[i][j-1]);
        __capc_rt[0]+=omp_get_wtime()-__capc_rs_0;
        __capc_rc[0]++;
        #pragma capc profitability_region end

        // reset dt
        dt = 0.0;

        // compute the largest change and copy T_new to T
        #pragma capc profitability_region begin
        double __capc_rs_1=omp_get_wtime();
        #pragma omp parallel for collapse(2) private(i,j) reduction(max:dt)
        for(i = 1; i <= GRIDX; i++){
            for(j = 1; j <= GRIDY; j++){
	      dt = MAX( fabs(T_new[i][j]-T[i][j]), dt);
	      T[i][j] = T_new[i][j];
            }
        }
        __capc_rt[1]+=omp_get_wtime()-__capc_rs_1;
        __capc_rc[1]++;
        #pragma capc profitability_region end

        // periodically print largest change
        if((iteration % 100) == 0) 
            printf("Iteration %4.0d, dt %f\n",iteration,dt);
        
	iteration++;
    }

    gettimeofday(&stop_time,NULL);
    timersub(&stop_time, &start_time, &elapsed_time); // measure time

    printf("Total time was %f seconds.\n", elapsed_time.tv_sec+elapsed_time.tv_usec/1000000.0);

    return 0;
}


// initialize grid and boundary conditions
void init(){

    int i,j;
    #pragma capc profitability_region begin
    double __capc_rs_2=omp_get_wtime();
    #pragma omp parallel for collapse(2) private(i,j)
    for(i = 0; i <= GRIDX+1; i++){
        for (j = 0; j <= GRIDY+1; j++){
            T[i][j] = 0.0;
        }
    }
    __capc_rt[2]+=omp_get_wtime()-__capc_rs_2;
    __capc_rc[2]++;
    #pragma capc profitability_region end

    // these boundary conditions never change throughout run

    // set left side to 0 and right to a linear increase
    #pragma capc profitability_region begin
    double __capc_rs_3=omp_get_wtime();
    #pragma omp parallel for private(i)
    for(i = 0; i <= GRIDX+1; i++) {
        T[i][0] = 0.0;
        T[i][GRIDY+1] = (128.0/GRIDX)*i;
    }
    __capc_rt[3]+=omp_get_wtime()-__capc_rs_3;
    __capc_rc[3]++;
    #pragma capc profitability_region end
    
    // set top to 0 and bottom to linear increase
    #pragma capc profitability_region begin
    double __capc_rs_4=omp_get_wtime();
    #pragma omp parallel for private(j)
    for(j = 0; j <= GRIDY+1; j++) {
        T[0][j] = 0.0;
        T[GRIDX+1][j] = (128.0/GRIDY)*j;
    }
    __capc_rt[4]+=omp_get_wtime()-__capc_rs_4;
    __capc_rc[4]++;
    #pragma capc profitability_region end
}