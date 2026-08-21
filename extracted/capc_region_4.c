#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
void init();
#define GRIDY    1000000000
#define GRIDX    4096
#define NUM_ITERATIONS 100
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MAX_TEMP_ERROR 0.02

void capc_region_4(double (* restrict T)[1000000002])
{
    int j;
    for(j = 0; j <= GRIDY+1; j++) {
        T[0][j] = 0.0;
        T[GRIDX+1][j] = (128.0/GRIDY) * j;
    }

}
