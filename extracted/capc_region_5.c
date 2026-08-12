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

void capc_region_5(double (* restrict RHODOT), double (* restrict VXDOT), double (* restrict VYDOT), double (* restrict DVXDX), double (* restrict DVXDY), double (* restrict DVYDX), double (* restrict DVYDY), double (* restrict VXBAR), double (* restrict VYBAR), double (* restrict FX), double (* restrict FY), double (* restrict EDOT), double* restrict rho0rho, double (* restrict RHO), double (* restrict P), double (* restrict rxydot), double* restrict EINT)
{
    int i;
    for(i=0;i<N;i++)    {
        RHODOT[i]   = 0.0;
        VXDOT[i]    = VYDOT[i] = 0.0;
        DVXDX[i]    = DVXDY[i] = DVYDX[i] = DVYDY[i] = 0.0;
        VXBAR[i]    = 0.0;
        VYBAR[i]    = 0.0;
        FX[i]       = FY[i] = 0.0;
        EDOT[i]     = 0.0;
        (*rho0rho)     = RHO0/RHO[i];
        P[i]        = -6.0*M*RHO0*( pow(2.0-(*rho0rho),2*M-1) - pow(2.0-(*rho0rho),M-1) );
        rxydot[i]   = 0.0;
        (*EINT)        = 0.0;
    }

}
