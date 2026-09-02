#include <stdio.h>
#include <math.h>
#include <stdlib.h>
void initialize();
void derivatives();
void kernel(double wf[], double dist);
void Update(double dt);
void plasticity();
    double ci = sqrt(EMOD / RHO[i]);
    double cj = sqrt(EMOD / RHO[j]);
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

void capc_region_11(int plate_particles, double* restrict angle, double pi, double* restrict radius, double (* restrict X), double (* restrict Y), double (* restrict VX), double (* restrict VY), double (* restrict E), double (* restrict MASS), double (* restrict RHO), double (* restrict SXX), double (* restrict SXY), double (* restrict SYY), double (* restrict DAMAGE), double (* restrict DeltaWP))
{
    int i;
    for(i = 0; i < plate_particles; i++)
    {
        (*angle) =
            2.0 * pi *
            (double)i /
            (double)plate_particles;

        (*radius) =
            ri +
            (ro-ri) *
            ((double)(i % 100) / 100.0);


        X[i] =
            (*radius) * cos((*angle));

        Y[i] =
            (*radius) * sin((*angle));


        VX[i] = VRING;
        VY[i] = 0.0;

        E[i] = 0.0;

        MASS[i] = MASSP;

        RHO[i] = RHO0;

        SXX[i] = 0.0;
        SXY[i] = 0.0;
        SYY[i] = 0.0;

        DAMAGE[i] = 0.0;

        DeltaWP[i] = 0.0;
    }

}
