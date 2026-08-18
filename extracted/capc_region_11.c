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
#define NB      1000000000
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

void capc_region_11(double* restrict sigxxi, double (* restrict SXX), double (* restrict P), double* restrict sigyyi, double (* restrict SYY), double* restrict sigxyi, double (* restrict SXY), double* restrict rho2i, double (* restrict RHO), double* restrict xij, double (* restrict X), double* restrict yij, double (* restrict Y), double* restrict vxij, double (* restrict VX), double* restrict vyij, double (* restrict VY), double* restrict dist, double hsq, double* restrict mata, double (* restrict wfgrad)[5], double* restrict matb, double* restrict matc, double* restrict matd, double (* restrict wf), double* restrict wf_grad, double (* restrict MASS), double* restrict wfdx, double* restrict wfdy, double* restrict rhobar, double* restrict drhobar, double (* restrict RHODOT), double* restrict sigxxj, double* restrict sigyyj, double* restrict sigxyj, double* restrict rho2j, double (* restrict rxydot), double (* restrict visc), double* restrict trace, double (* restrict DVXDX), double (* restrict DVYDY), double (* restrict SXXDOT), double (* restrict SYYDOT), double (* restrict SXYDOT), double (* restrict DVXDY), double (* restrict DVYDX), double (* restrict XDOT), double (* restrict VXBAR), double (* restrict YDOT), double (* restrict VYBAR), double (* restrict EDOT))
{
    int i;
    int j;
    for(i=N-NB;i<N;i++)    {
        (*sigxxi) = SXX[i] + P[i];
        (*sigyyi) = SYY[i] + P[i];
        (*sigxyi) = SXY[i];
        (*rho2i)  = 1.0/(RHO[i]*RHO[i]);
        for(j=N-NB;j<N;j++)    {
            if(i!=j)    {
                (*xij)  = X[i] - X[j];
                (*yij)  = Y[i] - Y[j];
                (*vxij) = VX[i] - VX[j];
                (*vyij) = VY[i] - VY[j];
                (*dist) = (*xij)*(*xij) + (*yij)*(*yij);
                if((*dist)<=hsq)   {
                    (*dist)        = sqrt((*dist));
                        (*mata) = wfgrad[i][1];
                        (*matb) = wfgrad[i][2];
                        (*matc) = wfgrad[i][3];
                        (*matd) = wfgrad[i][4];                    
                    kernel(wf, (*dist));
                    (*wf_grad) 	= wf[0]*MASS[j]/RHO[j]/wfgrad[i][0];
                    (*wfdx)		= 1.0/((*mata)*(*matd)-(*matb)*(*matc))*(wfgrad[i][4]*wf[1]*(*xij)/(*dist) - wfgrad[i][2]*wf[1]*(*yij)/(*dist));
                    (*wfdy)		= 1.0/((*mata)*(*matd)-(*matb)*(*matc))*(wfgrad[i][1]*wf[1]*(*yij)/(*dist) - wfgrad[i][3]*wf[1]*(*xij)/(*dist));
                    //(*wfdx)		= 0.50/((*mata)*(*matd)-(*matb)*(*matc))*(wfgrad[i][4]*wf[1]*(*xij)/(*dist) - wfgrad[i][2]*wf[1]*(*yij)/(*dist));
                    //(*wfdy)		= 0.50/((*mata)*(*matd)-(*matb)*(*matc))*(wfgrad[i][1]*wf[1]*(*yij)/(*dist) - wfgrad[i][3]*wf[1]*(*xij)/(*dist));

                    //(*mata) = wfgrad[j][1];
                    //(*matb) = wfgrad[j][2];
                    //(*matc) = wfgrad[j][3];
                    //(*matd) = wfgrad[j][4];
                    //(*wfdx)		-= 0.50/((*mata)*(*matd)-(*matb)*(*matc))*(-1.0*wfgrad[j][4]*wf[1]*(*xij)/(*dist) + wfgrad[j][2]*wf[1]*(*yij)/(*dist));
                    //(*wfdy)		-= 0.50/((*mata)*(*matd)-(*matb)*(*matc))*(-1.0*wfgrad[j][1]*wf[1]*(*yij)/(*dist) + wfgrad[j][3]*wf[1]*(*xij)/(*dist));
                    	                    
                    (*rhobar)      = RHO[j];
                    (*drhobar)     = (*dist)*(*rhobar);
                    //RHODOT[i]   += wf[1]*MASS[j]*((*vxij)*(*xij) + (*vyij)*(*yij))/(*dist);
                    RHODOT[i]   += MASS[j]*((*vxij)*(*wfdx) + (*vyij)*(*wfdy));
                    
                    (*sigxxj)      = SXX[j] + P[j];
                    (*sigyyj)      = SYY[j] + P[j];
                    (*sigxyj)      = SXY[j];
                    (*rho2j)       = 1.0/(RHO[j]*RHO[j]);

                    if(MONA_CORR == 1)  monacorr(i,j,wf,(*vxij),(*vyij),(*wf_grad),(*wfdx),(*wfdy));
                    if(JAUMANN == 1) rxydot[i] += -0.5 * MASS[j]/RHO[j]*((*vxij)*(*wfdy) - (*vyij)*(*wfdx));
                    
                    basicsph(i,j,wf,(*xij),(*yij),(*vxij),(*vyij),(*rho2i),(*rho2j),(*dist),(*sigxxi),(*sigyyi),(*sigxyi),(*sigxxj),(*sigyyj),(*sigxyj),(*drhobar),(*wf_grad),(*wfdx),(*wfdy));
                    
                    if(ART_VISCOSITY == 1)  viscosity(visc,i,j,(*xij),(*yij),(*vxij),(*vyij),(*dist),wf,(*wf_grad),(*wfdx),(*wfdy));
                    if(TENSILE == 1)  artificial_pressure(P[i],P[j],(*rho2i),(*rho2j),wf, (*xij), (*yij), (*dist), i, j, (*vxij), (*vyij),(*wf_grad),(*wfdx),(*wfdy));
                }
            }
        }
        (*trace)       = -1.0/3.0*(DVXDX[i] + DVYDY[i]);
        SXXDOT[i]   = 2.0*G*(DVXDX[i] + (*trace)) + 2.0*SXY[i]*rxydot[i];
        SYYDOT[i]   = 2.0*G*(DVYDY[i] + (*trace)) - 2.0*SXY[i]*rxydot[i];
        SXYDOT[i]   = 1.0*G*(DVXDY[i] + DVYDX[i]) - rxydot[i]*(SXX[i] - SYY[i]);
        XDOT[i] = VX[i] + VXBAR[i];
        YDOT[i] = VY[i] + VYBAR[i];
        EDOT[i] = -0.5*EDOT[i];
    }

}
