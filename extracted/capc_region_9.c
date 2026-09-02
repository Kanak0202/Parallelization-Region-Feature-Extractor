#include <stdio.h>
#include <math.h>
#include <stdlib.h>
void initialize();
void derivatives();
void kernel(double wf[], double dist);
void Update(double dt);
void plasticity();
#define N       1000000000
#define NB      (N/2)
#define DY      1.0
#define RHO0    1.1547
#define MASSP   1.00
#define MASSB   1.00
#define VRING   0.10
#define SIGMA   0.80
#define EPS     50.0
#define RADIUS  3
#define MAX     200.0
#define DT      0.01
#define H       3.0
#define M       2.0
#define EMOD    18.470
#define G       6.928
#define B       13.856
#define YIELD   1.360
#define ULTI    4.270
#define alpha   0.50
#define beta    0.50
#define ART_VISCOSITY 1
#define MONA_CORR     1
#define MONA_CORR2    2
#define JAUMANN       1
#define TENSILE       1
#define GRADCORR      1
#define ri      15.00
#define ro      20.00

void capc_region_9(double (* restrict VX), double (* restrict VXBAR), double (* restrict VY), double (* restrict VYBAR))
{
    int i;
    for(i = 0; i < N; i++)
    {
        if(MONA_CORR2 == 1)
        {
            VX[i] = VX[i] + VXBAR[i];
            VY[i] = VY[i] + VYBAR[i];
        }
    }

}
