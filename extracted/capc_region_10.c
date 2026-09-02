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

void capc_region_10(double (* restrict RHON), double (* restrict RHO), double dt, double (* restrict RHODOT), double (* restrict EN), double (* restrict E), double (* restrict EDOT), double (* restrict XN), double (* restrict X), double (* restrict XDOT), double (* restrict YN), double (* restrict Y), double (* restrict YDOT), double (* restrict VXN), double (* restrict VX), double (* restrict VXDOT), double (* restrict VYN), double (* restrict VY), double (* restrict VYDOT), double (* restrict SXXN), double (* restrict SXX), double (* restrict SXXDOT), double (* restrict SXYN), double (* restrict SXY), double (* restrict SXYDOT), double (* restrict SYYN), double (* restrict SYY), double (* restrict SYYDOT))
{
    int i;
    for(i = 0; i < N; i++)
    {
        RHON[i] =
            RHO[i] +
            0.5 * dt * RHODOT[i];

        EN[i] =
            E[i] +
            0.5 * dt * EDOT[i];

        XN[i] =
            X[i] +
            0.5 * dt * XDOT[i];

        YN[i] =
            Y[i] +
            0.5 * dt * YDOT[i];

        VXN[i] =
            VX[i] +
            0.5 * dt * VXDOT[i];

        VYN[i] =
            VY[i] +
            0.5 * dt * VYDOT[i];

        SXXN[i] =
            SXX[i] +
            0.5 * dt * SXXDOT[i];

        SXYN[i] =
            SXY[i] +
            0.5 * dt * SXYDOT[i];

        SYYN[i] =
            SYY[i] +
            0.5 * dt * SYYDOT[i];
    }

}
