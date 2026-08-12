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

void capc_region_4(double* restrict KE1, double (* restrict MASS), double (* restrict VX), double (* restrict VY), double* restrict IE1, double (* restrict E), double* restrict KE2, double* restrict IE2, double* restrict WP1, double (* restrict DeltaWP))
{
    int i;
            for(i=0; i<N; i++) {
                if(i< N-NB) {
                    (*KE1) += 0.5 * MASS[i] * ( VX[i]*VX[i] + VY[i]*VY[i] );
                    (*IE1) += MASS[i]* E[i];
                }
                if(i>= N-NB)    {
                    (*KE2) += 0.5 * MASS[i] * ( VX[i]*VX[i] + VY[i]*VY[i] );
                    (*IE2) += MASS[i]* E[i];
                }
                (*WP1) += DeltaWP[i];
            }

}
