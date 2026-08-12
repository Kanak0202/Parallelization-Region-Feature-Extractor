#include <stdio.h>
#include <math.h>
#include <stdlib.h>
void initialize();
void derivatives();
void kernel(double wf[], double dist );
void Update(double dt);
void callprint(int loopcounter);
void plasticity();
void viscosity(double visc[], int i, int j, double xij, double yij, double vxij, double vyij, double dist, double wf[], double wf_grad, double wfdx, double wfdy);
void consistency();
void artificial_pressure(double Pi, double Pj, double rho2i, double rho2j, double wf[], double xij, double yij, double dist, int i, int j, double vxij, double vyij, double wf_grad, double wfdx, double wfdy);
void monacorr(int i, int j, double wf[], double vxij, double vyij, double wf_grad, double wfdx, double wfdy);
void basicsph(int i,int j,double wf[], double xij, double yij, double vxij, double vyij, double rho2i, double rho2j, double dist, double sigxxi, double sigyyi, double sigxyi, double sigxxj, double sigyyj, double sigxyj, double drhobar, double wf_grad, double wfdx, double wfdy);
#define NX      50
#define NY      50
#define DY      1.0
#define N       2558
#define NB      50000
#define RHO0    1.1547
#define MASSP   1.00 //Comes from consistency
#define MASSB   1.00
#define VRING   0.10//-04.0
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
#define NPRINT  5
#define alpha   0.50
#define beta    0.50
#define ART_VISCOSITY 1
#define MONA_CORR     1
#define MONA_CORR2    2
#define JAUMANN      1
#define TENSILE       1
#define GRADCORR      1
#define ri      15.00
#define ro      20.00

void capc_region_14(double* restrict sigxx, double (* restrict SXX), double (* restrict P), double* restrict sigyy, double (* restrict SYY), double* restrict sigxy, double (* restrict SXY), double* restrict shear, double* restrict factor, double* restrict sum, double* restrict diff, double (* restrict DeltaWP), double (* restrict MASS), double (* restrict RHO), double (* restrict DAMAGE))
{
    int i;
    for(i=0;i<N;i++)  {
        (*sigxx) = SXX[i] + P[i];
        (*sigyy) = SYY[i] + P[i];
        (*sigxy) = SXY[i];
        (*shear) = sqrt((*sigxy)*(*sigxy) + 0.25*((*sigxx) - (*sigyy))*((*sigxx) - (*sigyy)) );

        if((*shear) > YIELD)   {
            (*factor) = YIELD/(*shear);
            (*sum) = (*sigxx) + (*sigyy);
            (*diff) = (*sigxx) - (*sigyy);
            (*sigxy)  = (*factor)*(*sigxy);
            (*sigxx) = 0.5*(*sum) + 0.5*(*factor)*(*diff);
            (*sigyy) = 0.5*(*sum) - 0.5*(*factor)*(*diff);
            DeltaWP[i] = (1.0 - (*factor))*(*factor)*(2.0*SXY[i]*SXY[i] + SXX[i]*SXX[i] + SYY[i]*SYY[i])*MASS[i]/3.00/G/RHO[i];
        }

        if(0.5*((*sigxx) + (*sigyy)) > ULTI)  {
            (*sigxy) = 0.0;
            (*sigxx) = P[i];
            (*sigyy) = P[i];
            RHO[i] = RHO0;
            DAMAGE[i] = 0.0;
        }
        SXX[i] = (*sigxx) - P[i];
        SYY[i] = (*sigyy) - P[i];
        SXY[i] = (*sigxy);
    }

}
