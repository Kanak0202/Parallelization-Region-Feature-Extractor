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

void capc_region_6(double* restrict xij, double (* restrict X), double* restrict yij, double (* restrict Y), double* restrict dist, double hsq, double (* restrict wf), double (* restrict wfgrad)[5], double (* restrict MASS), double (* restrict RHO))
{
    int i;
    int j;
        for(i = 0; i < N-NB; i++)
        {
            for(j = 0; j < N-NB; j++)
            {
                if(i != j)
                {
                    (*xij) = X[i] - X[j];
                    (*yij) = Y[i] - Y[j];

                    (*dist) = (*xij)*(*xij) + (*yij)*(*yij);

                    if((*dist) <= hsq)
                    {
                        (*dist) = sqrt((*dist));

                        kernel(wf, (*dist));

                        wfgrad[i][0] +=
                            wf[0] * MASS[j] / RHO[j];

                        wfgrad[i][1] +=
                            -1.0 * (*xij) * wf[1] *
                            (*xij) / (*dist) * MASS[j] / RHO[j];

                        wfgrad[i][2] +=
                            -1.0 * (*yij) * wf[1] *
                            (*xij) / (*dist) * MASS[j] / RHO[j];

                        wfgrad[i][3] +=
                            -1.0 * (*xij) * wf[1] *
                            (*yij) / (*dist) * MASS[j] / RHO[j];

                        wfgrad[i][4] +=
                            -1.0 * (*yij) * wf[1] *
                            (*yij) / (*dist) * MASS[j] / RHO[j];
                    }
                }
            }
        }

}
