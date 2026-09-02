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

void capc_region_13(double (* restrict XO), double (* restrict X), double (* restrict YO), double (* restrict Y), double (* restrict VXO), double (* restrict VX), double (* restrict VYO), double (* restrict VY), double (* restrict RHOO), double (* restrict RHO), double (* restrict EO), double (* restrict E), double (* restrict SXXO), double (* restrict SXX), double (* restrict SXYO), double (* restrict SXY), double (* restrict SYYO), double (* restrict SYY), double (* restrict XN), double (* restrict YN), double (* restrict VXN), double (* restrict VYN), double (* restrict RHON), double (* restrict EN), double (* restrict SXXN), double (* restrict SXYN), double (* restrict SYYN))
{
    int i;
    for(i = 0; i < N; i++)
    {
        XO[i] = X[i];
        YO[i] = Y[i];

        VXO[i] = VX[i];
        VYO[i] = VY[i];

        RHOO[i] = RHO[i];

        EO[i] = E[i];

        SXXO[i] = SXX[i];
        SXYO[i] = SXY[i];
        SYYO[i] = SYY[i];

        XN[i] = X[i];
        YN[i] = Y[i];

        VXN[i] = VX[i];
        VYN[i] = VY[i];

        RHON[i] = RHO[i];

        EN[i] = E[i];

        SXXN[i] = SXX[i];
        SXYN[i] = SXY[i];
        SYYN[i] = SYY[i];
    }

}
